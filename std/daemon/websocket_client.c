/**
 * @file /std/daemon/websocket_client.c
 * @description This is a base WebSocket client daemon that is intended to be
 *              inherited by other objects to create a custom WS client. It
 *              provides the ability to send start an HTTP session and then
 *              upgrade to a WebSocket connection to then send/receive
 *              messages.
 *
 * @created 2024-07-05 - Gesslar
 * @last_modified 2024-07-05 - Gesslar
 *
 * @history
 * 2024-07-05 - Gesslar - Created
 */

#include <daemons.h>
#include <websocket.h>
#include <socket.h>
#include <socket_err.h>

inherit STD_DAEMON ;
inherit M_HTTP ;

// Functions
private nomask string random_string(int length) ;
protected nomask mapping websocket_connect(string url) ;

private nomask void process_handshake(buffer buf) ;
private nomask int is_message_complete(buffer buf) ;
private nomask mapping parse_websocket_frame(buffer buf) ;
private nomask void process_websocket_message(mapping frame_info) ;
private nomask buffer apply_mask(buffer data, buffer mask) ;
varargs protected nomask int websocket_close(int code, string reason) ;
protected nomask void shutdown_websocket() ;

private string unformat_frame(buffer message) ;
private nomask varargs buffer format_frame(int opcode, mixed args...) ;

protected nomask varargs int websocket_message(int frame_opcode, mixed args...) ;
protected nomask void send_pong(string payload) ;
protected nomask void send_ping() ;

void shutdown() ;
void shutdown_force() ;

// Variables
protected nosave nomask mapping server ;
private nosave nomask string *subprotocols, *charset ;

void mudlib_setup() {
    set_log_level(0) ;
    subprotocols = ({ }) ;

    charset = explode(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcde"
        "fghijklmnopqrstuvwxyz0123456789",
    "") ;
}

/**
 * Initiates a WebSocket connection to the specified URL.
 *
 * @param {string} url - The URL to connect to.
 */
protected nomask mapping websocket_connect(string url) {
    string handshake_key ;
    int fd, resolve_key ;
    mapping parsed_url ;

    _log(2, "Connecting to WebSocket: %s", url) ;

    _log(3, "Server: %O", server) ;

    if(mapp(server))
        return ([
            "status" : "error",
            "error" : "Already connected to a WebSocket server",
        ]) ;

    if(!stringp(url)) {
        return ([
            "status" : "error",
            "error" : "Invalid URL",
        ]) ;
    }

    parsed_url = parse_url(url) ;
    if(!parsed_url) {
        _log(2, "Failed to parse URL: %s", url) ;
        return ([
            "status" : "error",
            "error" : "Failed to parse URL",
        ]) ;
    }

    handshake_key = random_string(16); // Raw random string

    server = ([
        "start_time" : time_frac(),
        "handshake_key" : handshake_key,
        "request" : parsed_url + ([
            "subprotocols" : subprotocols,
        ]),
    ]) ;

    _log(3, "Creating socket for %s", server["request"]["host"]) ;

    fd = socket_create(
        server["request"]["secure"] ? STREAM_TLS_BINARY : STREAM_BINARY,
        "websocket_read",
        "websocket_closed"
    ) ;

    if(fd < 0) {
        _log(0, "Unable to create socket: %s", socket_error(fd)) ;
        server = null ;
        return ([
            "status" : "error",
            "error" : socket_error(fd),
        ]) ;
    }

    _log(4, "Socket created: %d", fd) ;

    if(fd < 0) {
        _log(0, "Unable to create socket: %s", socket_error(fd)) ;
        return ([
            "error" : socket_error(fd),
        ]) ;
    }

    if(server["request"]["secure"]) {
        socket_set_option(fd, SO_TLS_VERIFY_PEER, 1) ;
        socket_set_option(fd, SO_TLS_SNI_HOSTNAME, server["request"]["host"]) ;
    }

    server["websocket_state"] = WS_STATE_RESOLVING ;
    _log(3, "Request: %O", server["request"]) ;

    resolve_key = resolve(server["request"]["host"], "websocket_resolved") ;
    server["fd"] = fd ;
    server["key"] = resolve_key ;

    _log(3, "Server: %O", server) ;

    return ([
        "status" : "success",
        "fd" : fd,
    ]) ;
}

/**
 * Handles the resolution of the WebSocket host.
 *
 * @param {string} host - The host that was resolved.
 * @param {string} addr - The resolved address.
 * @param {int} key - The key associated with the resolution request.
 */
protected nomask void websocket_resolved(string host, string addr, int key) {
    int fd, result ;

    _log(2, "Resolving WebSocket host: %s", host) ;

    if(!server)
        return ;

    fd = server["fd"] ;

    map_delete(server, "key") ;

    _log(4, "Socket resolved: %d", fd) ;

    if(addr) {
        int port = server["request"]["port"] ;

        _log(2, "Host resolved: %s (%s)", host, addr) ;

        server["ip"] = addr ;
        server["state"] = WS_STATE_CONNECTING ;

        call_if(this_object(), "websocket_handle_resolved") ;

        result = socket_connect(fd, addr + " " + port, "websocket_read", "websocket_ready") ;
        if(result != EESUCCESS) {
            _log(1, "Error connecting to %s: %s", host, socket_error(result)) ;

            server["state"] = WS_STATE_ERROR ;
            server["error"] = socket_error(result) ;

            call_if(this_object(), "websocket_handle_connection_error", result) ;

            shutdown_websocket() ;
            return ;
        }

        server["state"] = WS_STATE_HANDSHAKE ;
        server["transactions"] = 0 ;
        server["received_total"] = 0 ;

        _log(2, "Connected to %s %d", host, port) ;
    } else {
        _log(0, "Failed to resolve hostname %s", host) ;

        server["state"] = WS_STATE_ERROR ;
        server["error"] = "Failed to resolve hostname " + host ;

        call_if(this_object(), "websocket_handle_resolve_error") ;

        shutdown_websocket() ;
        return ;
    }
}

/**
 * Handles the socket being ready for data transmission.
 *
 * @param {int} fd - The file descriptor of the socket.
 */
public nomask void websocket_ready(int fd) {
    string out ;
    string raw_key, sec_websocket_key ;
    string host ;
    int result ;
    string subs ;
    int written ;

    _log(2, "Socket fully connected, now able to send data") ;

    if(!server) {
        _log(0, "No server data for fd %d", fd) ;
        return ;
    }

    server["state"] = WS_STATE_HANDSHAKE ;

    if(!stringp(raw_key = server["handshake_key"])) {
        _log(0, "No handshake key for fd %d", fd) ;
        shutdown_websocket() ;
        return ;
    }

    if(!server["request"]["host"]) {
        _log(0, "No host specified for fd %d", fd) ;
        shutdown_websocket() ;
        return ;
    }

    sec_websocket_key = base64_encode(to_binary(raw_key)) ;
    _log(2, "Sec-WebSocket-Key Sent to %s: %s", sec_websocket_key, server["request"]["host"]) ;

    out = sprintf(
        "GET "+server["request"]["path"]+" HTTP/1.1\r\n"
        "Host: "+server["request"]["host"]+"\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n",
        sec_websocket_key
    ) ;

    if(sizeof(server["subprotocols"])) {
        subs = implode(server["subprotocols"], ", ") ;
        subs = sprintf("Sec-WebSocket-Protocol: %s\r\n", subs) ;
    }

    if(strlen(subs))
        out += subs ;

    out += "\r\n" ;

    _log(2, "Sending handshake") ;
    _log(3, "Handshake request: %s", identify(out)) ;

    written = sizeof(to_binary(out)) ;
    server["sent_total"] = written ;
    result = socket_write(fd, out) ;
    if(result != EESUCCESS) {
        _log(2, "Failed to send handshake request: %d", result) ;
        call_if(this_object(), "websocket_handle_handshake_error", result) ;
        shutdown_websocket() ;
    } else {
        call_if(this_object(), "websocket_handle_handshake_sent") ;
        _log(2, "Handshake request sent") ;
    }
}

/**
 * Handles the closure of the WebSocket.
 *
 * @param {int} fd - The file descriptor of the socket.
 */
protected nomask void websocket_closed(int fd) {
    float duration ;
    float speed ;

    _log(3, "Socket closed for fd %d", fd) ;

    if(!server)
        return ;

    _log(3, "Socket closed: %s %d", server["request"]["host"], server["request"]["port"]) ;

    call_if(this_object(), "websocket_handle_closed") ;

    shutdown_websocket(fd) ;
}

/**
 * Shuts down the WebSocket connection and logs the statistics.
 */
protected nomask void shutdown_websocket() {
    int result ;
    float duration ;
    float now, started ;
    float received_total ;
    int fd ;

    if(!server)
    return ;

    fd = server["fd"] ;

    _log(3, "Shutting down socket for %d", fd) ;

    now = time_frac() ;
    started = to_float(server["start_time"]) ;

    duration = (now - started ) ;

    received_total = to_float(server["received_total"]) ;

    _log(3, "Total data sent: %d bytes", server["sent_total"]) ;
    _log(3, "Total transactions: %d", server["transactions"]) ;
    _log(3, "Total data received: %d bytes", server["received_total"]) ;
    _log(3, "Uptime: %.2f seconds", duration) ;

    call_if(this_object(), "websocket_handle_shutdown") ;

    result = socket_close(fd) ;
    if(result != EESUCCESS) {
        _log(0, "Error closing socket: %s", socket_error(result)) ;
    } else {
        _log(4, "Socket removed: %s %d", server["request"]["host"], server["request"]["port"]) ;

    }

    server = null ;
}

/**
 * Reads incoming data from the WebSocket.
 *
 * @param {int} fd - The file descriptor of the socket.
 * @param {buffer} incoming - The incoming data buffer.
 */
public nomask void websocket_read(int fd, buffer incoming) {
    buffer buf ;
    mapping frame_info ;
    mapping http ;

    if(!server)
        return ;

    server["transactions"]++ ;

    _log(3, "===========  STARTING WS TRANSACTION %d  ===========", server["transactions"]) ;

    _log(4, "Typeof incoming: %s", typeof(incoming)) ;
    _log(4, "Typeof buffer: %s", typeof(server["buffer"])) ;

    if(server["buffer"]) {
        buf = server["buffer"] + incoming ;
        map_delete(server, "buffer") ;
    } else {
        buf = incoming ;
    }

    server["received_total"] += sizeof(buf) ;

    _log(3, "Incoming size: %d", sizeof(incoming)) ;
    _log(3, "Buffer size: %d", sizeof(buf)) ;

    // Process headers if not done yet
    server["response"] = server["response"] || ([]) ;

    if(sizeof(buf) && !server["response"]["status"]) {
        mapping status ;

        status = parse_response_status(buf, 1) ;
        if(!status) {
            server["buffer"] = buf ;
            _log(4, "Type of buffer: %s", typeof(server["buffer"])) ;
            return ;
        }

        if(status["buffer"]) {
            buf = status["buffer"] ;
            map_delete(status, "buffer") ;
        }

        server["response"]["status"] = status ;

        _log(3, "Status found: %O", status) ;
    }

    if(sizeof(buf) && !server["response"]["headers"]) {
        mapping headers ;

        headers = parse_headers(buf, 1) ;

        if(!headers) {
            server["buffer"] = buf ;
            _log(4, "Type of buffer: %s", typeof(server["buffer"])) ;
            return ;
        }

        if(headers["buffer"]) {
            buf = headers["buffer"] ;
            map_delete(headers, "buffer") ;
        }

        server["response"]["headers"] = headers ;

        _log(4, "Headers found: %O", headers) ;
    }

    if(server["state"] == WS_STATE_HANDSHAKE) {
        process_handshake(buf) ;
        return ;
    }

    if(server["state"] == WS_STATE_CONNECTED) {
        _log(2, "Processing WebSocket data") ;

        // Handle WebSocket data frames
        while(is_message_complete(buf)) {
            frame_info = parse_websocket_frame(buf) ;
            if(frame_info) {
                process_websocket_message(frame_info) ;
                if(!server)
                    return ;
                buf = frame_info["buffer"] ;
            } else {
                break ;
            }
        }

        // Save any remaining buffer data for next processing
        server["buffer"] = buf ;
        _log(4, "Type of buffer: %s", typeof(server["buffer"])) ;

        // Continue processing if there's still data left
        if(sizeof(buf))
            _log(2, "Data left in buffer for next transaction.") ;
    }

    _log(3, "Final buffer size: %d", sizeof(buf)) ;
    _log(3, "===========  ENDING WS TRANSACTION %d  ===========", server["transactions"]) ;
}

/**
 * Processes the WebSocket handshake.
 *
 * @param {buffer} buf - The buffer containing the handshake data.
 */
private nomask void process_handshake(buffer buf) {
    string raw_key = server["handshake_key"] ;
    string sec_websocket_key, concat, hash_result_hex, expected, accept ;
    buffer hash_result_binary ;
    mapping frame_info ;
    mapping response = server["response"] ;

    _log(2, "Validating handshake") ;
    _log(4, "Server: %O", server) ;
    // _log(4, "Buffer: %O", buf) ;

    _log(2, "HTTP Status Code: %d", server["response"]["status"]["code"]) ;
    _log(2, "Header 'upgrade': %s", server["response"]["headers"]["upgrade"]) ;
    _log(2, "Header 'connection': %O", identify(server["response"]["headers"]["connection"])) ;

    // Encode the raw key
    sec_websocket_key = base64_encode(to_binary(raw_key)) ;
    _log(3, "Generated Key: %s", raw_key) ;

    // Concatenate base64 encoded key and WebSocket GUID
    concat = sec_websocket_key + WS_HANDSHAKE_GUID ;

    // Compute SHA-1 hash
    hash_result_hex = hash("sha1", concat) ;

    // Convert hex to binary
    hash_result_binary = hex_to_binary(hash_result_hex) ;

    // Base64 encode the binary hash result
    expected = base64_encode(hash_result_binary) ;
    _log(3, "Base64 Encoded SHA-1 Hash (Expected): %s", expected) ;

    // Accept value from server
    accept = server["response"]["headers"]["sec-websocket-accept"] ;
    _log(3, "Header 'sec-websocket-accept': %s", accept) ;

    // Compare expected and accept values
    if(accept != expected) {
        _log(1, "Accept and Expected do not match.") ;
        server["state"] = WS_STATE_HANDSHAKE_FAILED ;
        call_if(this_object(), "websocket_handle_handshake_failed", "Accept and Expected do not match.") ;
        shutdown_websocket() ;
        return ;
    }

    if(response["status"]["code"] != 101 ||
        response["headers"]["upgrade"] != "websocket" ||
        (member_array("Upgrade", response["headers"]["connection"]) == -1 &&
         member_array("upgrade", response["headers"]["connection"]) == -1) ||
        accept != expected) {
        _log(1, "Handshake invalid. Disconnecting.") ;
        server["state"] = WS_STATE_HANDSHAKE_FAILED ;
        call_if(this_object(), "websocket_handle_handshake_failed", "Handshake invalid. Disconnecting.") ;
        shutdown_websocket() ;
        return ;
    }

    _log(2, "Handshake validated") ;

    server["state"] = WS_STATE_CONNECTED ;

    _log(1, "Connected to %s", server["request"]["host"]) ;

    map_delete(server, "handshake_key") ;

    call_if(this_object(), "websocket_handle_connected") ;
}

/**
 * Checks if the buffer contains a complete WebSocket frame.
 *
 * @param {buffer} buf - The buffer to check.
 * @returns {int} - 1 if the message is complete, 0 otherwise.
 */
private nomask int is_message_complete(buffer buf) {
    int payload_length ;
    int offset ;

    if(sizeof(buf) < 2) {
        return 0 ;
    }

    payload_length = buf[1] & 0x7F ;
    offset = 2 ;

    if(payload_length == 126) {
        if(sizeof(buf) < 4) {
            _log(3, "Not enough data to determine the full payload length") ;
            return 0; // Not enough data to determine the full payload length
        }
        payload_length = (buf[2] << 8) | buf[3] ;
        offset = 4 ;
    } else if(payload_length == 127) {
        if(sizeof(buf) < 10) {
            _log(3, "Not enough data to determine the full payload length") ;
            return 0; // Not enough data to determine the full payload length
        }
        payload_length = ((buf[2] & 0xFF) << 56) | ((buf[3] & 0xFF) << 48) |
                         ((buf[4] & 0xFF) << 40) | ((buf[5] & 0xFF) << 32) |
                         ((buf[6] & 0xFF) << 24) | ((buf[7] & 0xFF) << 16) |
                         ((buf[8] & 0xFF) << 8)  |  (buf[9] & 0xFF) ;
        offset = 10 ;
    }

    // Handle masked payloads
    if(buf[1] & 0x80) {
        offset += 4 ;
    }

    // Check if we have enough data for the complete frame
    if(sizeof(buf) < offset + payload_length) {
        _log(3, "Not enough data for the full payload") ;
        return 0 ;
    }

    _log(3, "Enough data for the full payload") ;

    return 1 ;
}

/**
 * Parses a WebSocket frame from the buffer.
 *
 * @param {buffer} buf - The buffer containing the WebSocket frame.
 * @returns {mapping} - A mapping with the frame information.
 */
private nomask mapping parse_websocket_frame(buffer buf) {
    mapping result = ([]) ;
    int fin, opcode, masked, payload_length, offset ;
    buffer payload ;

    if(sizeof(buf) < 2) {
        _log(3, "Insufficient data to process") ;
        return 0 ;
    }

    fin = buf[0] & 0x80 ;
    opcode = buf[0] & 0x0F ;
    masked = buf[1] & 0x80 ;
    payload_length = buf[1] & 0x7F ;
    offset = 2 ;

    _log(3, "Initial frame details - fin: %d, opcode: %d, masked: %d, payload_length: %d", fin, opcode, masked, payload_length) ;

    if(payload_length == 126) {
        if(sizeof(buf) < 4) {
            _log(3, "Insufficient data for extended payload length (126)") ;
            return 0 ;
        }
        payload_length = (buf[2] << 8) | buf[3] ;
        offset = 4 ;
    } else if(payload_length == 127) {
        if(sizeof(buf) < 10) {
            _log(3, "Insufficient data for extended payload length (127)") ;
            return 0 ;
        }
        payload_length = ((buf[2] & 0xFF) << 56) | ((buf[3] & 0xFF) << 48) |
                         ((buf[4] & 0xFF) << 40) | ((buf[5] & 0xFF) << 32) |
                         ((buf[6] & 0xFF) << 24) | ((buf[7] & 0xFF) << 16) |
                         ((buf[8] & 0xFF) << 8) | (buf[9] & 0xFF) ;
        offset = 10 ;
    }

    _log(3, "Processed payload length: %d, offset: %d", payload_length, offset) ;

    if(masked)
        offset += 4 ;

    if(sizeof(buf) < offset + payload_length) {
        _log(3, "Insufficient data for full payload. Needed: %d, available: %d", offset + payload_length, sizeof(buf)) ;
        return 0 ;
    }

    payload = allocate_buffer(payload_length) ;

    if(masked) {
        buffer mask = buf[2..5] ;
        for(int i = 0; i < payload_length; i++) {
            payload[i] = buf[offset + i] ^ mask[i % 4] ;
        }
    } else {
        payload = buf[offset..offset + payload_length - 1] ;
    }

    _log(3, "Payload unmasked or copied") ;

    result["fin"] = fin ;
    result["opcode"] = opcode ;
    result["masked"] = masked ;
    result["payload_length"] = payload_length ;
    if(opcode == WS_TEXT_FRAME)
        result["payload"] = parse_body(to_string(payload), "auto") ;
    else
        result["payload"] = payload ;

    // Update the buffer to only contain unprocessed data
    result["buffer"] = buf[offset + payload_length..] ;

    _log(3, "Offset: %d, Payload Length: %d, Buffer Size: %d", offset, payload_length, sizeof(buf)) ;
    _log(3, "Remaining buffer size: %d", sizeof(result["buffer"])) ;

    return result ;
}

// WEBSOCKET HANDLERS
private nomask void process_text_frame(mapping frame_info) ;
private nomask void process_close_frame(mapping frame_info) ;
private nomask void process_ping_frame(mapping frame_info) ;
private nomask void process_pong_frame(mapping frame_info) ;
private nomask void process_continuation_frame(mapping frame_info) ;
private nomask void process_binary_frame(mapping frame_info) ;
private nomask void process_unknown_frame(mapping frame_info) ;

/**
 * Processes a WebSocket message based on the frame information.
 *
 * @param {int} fd - The file descriptor of the socket.
 * @param {mapping} frame_info - The frame information.
 */
private nomask void process_websocket_message(mapping frame_info) {
    int opcode ;
    mixed err ;

    opcode = frame_info["opcode"] ;

    // Process the frame based on the opcode
    if(opcode == WS_TEXT_FRAME) {
        _log(2, "Received text frame") ;
        _log(4, "Payload: %O", frame_info["payload"]) ;
        err = catch(process_text_frame(frame_info)) ;
    } else if(opcode == WS_CLOSE_FRAME) {
        _log(2, "Received close frame") ;
        _log(3, "Payload: %O", binary_to_hex(frame_info["payload"])) ;
        err = catch(process_close_frame(frame_info)) ;
    } else if(opcode == WS_PING_FRAME) {
        _log(2, "Received ping frame") ;
        err = catch(process_ping_frame(frame_info)) ;
    } else if(opcode == WS_PONG_FRAME) {
        _log(2, "Received pong frame") ;
        err = catch(process_pong_frame(frame_info)) ;
    } else if(opcode == WS_CONTINUATION_FRAME) {
        _log(2, "Received continuation frame") ;
        err = catch(process_continuation_frame(frame_info)) ;
    } else if(opcode == WS_BINARY_FRAME) {
        _log(2, "Received binary frame") ;
        err = catch(process_binary_frame(frame_info)) ;
    } else {
        _log(2, "Received unknown frame with opcode %d", opcode) ;
    }

    if(err) {
        _log(2, "Error processing frame: %O", err) ;
        shutdown_websocket() ;
    }

    if(!server)
        return ;

    // Save any remaining buffer data for next processing
    server["buffer"] = frame_info["buffer"] ;
    _log(4, "Type of buffer: %s", typeof(server["buffer"])) ;
}

/**
 * Processes an incoming text frame from the WebSocket.
 *
 * @param {mapping} frame_info - The frame information.
 */
private nomask void process_text_frame(mapping frame_info) {
    mapping payload = frame_info["payload"] ;

    _log(3, "Payload: %O", payload) ;

    call_if(this_object(), "websocket_handle_text_frame", payload) ;
}

/**
 * Processes an incoming close frame from the WebSocket.
 *
 * @param {mapping} frame_info - The frame information.
 */
private nomask void process_close_frame(mapping frame_info) {
    int close_code = 0 ;

    string close_reason = "<no reason given>" ;
    if(sizeof(frame_info["payload"]) >= 2) {
        close_code = (frame_info["payload"][0] << 8) | frame_info["payload"][1] ;
        if(sizeof(frame_info["payload"]) > 2)
            close_reason = to_string(frame_info["payload"][2..]) ;
    }
    _log(0, "Connection closed by server. Code: %d, Reason: %s", close_code, close_reason) ;

    call_if(this_object(), "websocket_handle_close_frame", frame_info["payload"]) ;
}

/**
 * Processes an incoming ping request frame from the WebSocket.
 *
 * @param {mapping} frame_info - The frame information.
 */
private nomask void process_ping_frame(mapping frame_info) {
    _log(2, "Received ping frame") ;
    send_pong(to_string(frame_info["payload"])) ;

    call_if(this_object(), "websocket_handle_ping_frame", frame_info) ;
}

/**
 * Processes an incoming pong frame from the WebSocket.
 *
 * @param {mapping} frame_info - The frame information.
 */
private nomask void process_pong_frame(mapping frame_info) {
    _log(2, "Received pong frame") ;

    call_if(this_object(), "websocket_handle_pong_frame", frame_info) ;
}

/**
 * Processes an incoming continuation frame from the WebSocket.
 *
 * @param {mapping} frame_info - The frame information.
 */
private nomask void process_continuation_frame(mapping frame_info) {
    _log(2, "Received continuation frame") ;

    call_if(this_object(), "websocket_handle_continuation_frame", frame_info) ;
}

/**
 * Processes an incoming binary frame from the WebSocket.
 *
 * @param {mapping} frame_info - The frame information.
 */
private nomask void process_binary_frame(mapping frame_info) {
    _log(2, "Received binary frame") ;

    call_if(this_object(), "websocket_handle_binary_frame", frame_info) ;
}

/**
 * Processes an unknown incoming frame from the WebSocket.
 *
 * @param {mapping} frame_info - The frame information.
 */
private nomask void process_unknown_frame(mapping frame_info) {
    _log(2, "Received unknown frame") ;

    call_if(this_object(), "websocket_handle_unknown_frame", frame_info) ;
}

/**
 * Formats a WebSocket frame based on the opcode and arguments.
 *
 * @param {int} opcode - The opcode for the frame.
 * @param {...mixed} args - The arguments for the frame.
 * @returns {buffer} - The formatted frame.
 */
private nomask varargs buffer format_frame(int opcode, mixed args...) {
    int len, i, frame_offset ;
    buffer frame ;
    buffer payload ;
    int close_code ;
    string reason ;
    buffer close_payload ;
    buffer mask ;
    buffer masked_payload ;

    if(!arrayp(args)) {
        args = ({ args }) ;
    }

    // Determine the payload based on the opcode
    switch(opcode) {
        case WS_CLOSE_FRAME: {
            if(sizeof(args) < 1 || !intp(args[0])) {
                _log(2, "Invalid close code: %O", args[0]) ;
                return 0 ;
            }
            close_code = args[0] ;
            reason = (sizeof(args) > 1 && stringp(args[1])) ? args[1] : "" ;
            close_payload = allocate_buffer(2 + strlen(reason)) ;
            close_payload[0] = (close_code >> 8) & 0xFF ;
            close_payload[1] = close_code & 0xFF ;
            close_payload[2..] = to_binary(reason) ;
            payload = close_payload ;
            break ;
        }
        case WS_TEXT_FRAME:
        case WS_BINARY_FRAME:
        case WS_PING_FRAME:
        case WS_PONG_FRAME:
            payload = to_binary(args[0]); // Convert string to buffer
            break ;
        default:
            _log(3, "Unsupported frame opcode: %d", opcode) ;
            return 0 ;
    }

    len = sizeof(payload) ;
    mask = allocate_buffer(4) ;

    // Generate a random masking key
    for(i = 0; i < 4; i++) {
        mask[i] = random(256) ;
    }

    if(len <= 125) {
        frame = allocate_buffer(2 + len + 4); // 2 bytes header + 4 bytes mask key + payload
        frame[1] = 0x80 | len; // Mask bit set + payload length
        frame_offset = 2; // Header size
    } else if(len <= 65535) {
        frame = allocate_buffer(4 + len + 4); // 2 bytes header + 2 bytes extended payload length + 4 bytes mask key + payload
        frame[1] = 0x80 | 126; // Mask bit set + 126
        frame[2] = (len >> 8) & 0xFF ;
        frame[3] = len & 0xFF ;
        frame_offset = 4; // Header size + extended payload length
    } else {
        frame = allocate_buffer(10 + len + 4); // 2 bytes header + 8 bytes extended payload length + 4 bytes mask key + payload
        frame[1] = 0x80 | 127; // Mask bit set + 127
        for(i = 0; i < 8; i++) {
            frame[2 + i] = (len >> (8 * (7 - i))) & 0xFF ;
        }
        frame_offset = 10; // Header size + extended payload length
    }

    frame[0] = 0x80 | opcode; // FIN bit set + opcode
    frame[frame_offset..(frame_offset + 3)] = mask; // Set the mask key

    // Mask the payload
    masked_payload = allocate_buffer(len) ;
    for(i = 0; i < len; i++) {
        masked_payload[i] = payload[i] ^ mask[i % 4] ;
    }
    frame[(frame_offset + 4)..] = masked_payload ;

    // Detailed logging
    _log(2, "Payload length: %d", len) ;
    _log(3, "Masking key: %O", binary_to_hex(mask)) ;
    _log(3, "Payload before masking: %O", binary_to_hex(payload)) ;
    _log(3, "Masked payload: %O", binary_to_hex(masked_payload)) ;
    _log(3, "Final frame: %O", binary_to_hex(frame)) ;

    return frame ;
}

/**
 * Applies a mask to the given buffer data.
 *
 * @param {buffer} data - The data buffer to be masked.
 * @param {buffer} mask - The masking key.
 * @returns {buffer} - The masked data buffer.
 */
private nomask buffer apply_mask(buffer data, buffer mask) {
    int length = sizeof(data) ;
    buffer masked_data = allocate_buffer(length) ;

    for(int i = 0; i < length; i++) {
        masked_data[i] = data[i] ^ mask[i % 4] ;
    }

    return masked_data ;
}

/**
 * Unformats a WebSocket frame from the given message buffer.
 *
 * @param {buffer} message - The message buffer containing the WebSocket frame.
 * @returns {string} - The unformatted frame as a string.
 */
private string unformat_frame(buffer message) {
    int len, i, frame_offset ;
    int payload_len ;
    buffer mask ;
    buffer payload ;
    buffer unmasked_payload ;
    int fin, opcode, mask_bit ;

    // Read the first byte (FIN bit and opcode)
    fin = message[0] & 0x80 ;
    opcode = message[0] & 0x0F ;

    // Read the second byte (Mask bit and payload length)
    mask_bit = message[1] & 0x80 ;
    payload_len = message[1] & 0x7F ;

    // Determine the payload length
    if(payload_len == 126) {
        payload_len = (message[2] << 8) | message[3] ;
        frame_offset = 4 ;
    } else if(payload_len == 127) {
        payload_len = 0 ;
        for(i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | message[2 + i] ;
        }
        frame_offset = 10 ;
    } else {
        frame_offset = 2 ;
    }

    // Read the masking key
    mask = message[frame_offset..(frame_offset + 3)] ;
    frame_offset += 4 ;

    // Read the masked payload
    payload = message[frame_offset..(frame_offset + payload_len - 1)] ;

    // Unmask the payload
    unmasked_payload = allocate_buffer(payload_len) ;
    for(i = 0; i < payload_len; i++) {
        unmasked_payload[i] = payload[i] ^ mask[i % 4] ;
    }

    // Convert the unmasked payload to a string and return it
    return to_string(unmasked_payload) ;
}

/**
 * Sends a WebSocket message with the specified frame opcode and arguments.
 *
 * @param {int} frame_opcode - The opcode for the frame.
 * @param {...mixed} args - The arguments for the frame.
 * @returns {int} - 1 if the message was sent successfully, 0 otherwise.
 */
protected nomask varargs int websocket_message(int frame_opcode, mixed args...) {
    buffer frame ;
    int result ;
    int written ;
    string unformatted_frame ;
    int fd ;

    if(!server)
        return 0 ;

    fd = server["fd"] ;

    if(member_array(frame_opcode, WS_VALID_FRAME_OPCODES) == -1) {
        _log(3, "Invalid opcode: %d", frame_opcode) ;
        return 0 ;
    }

    if(nullp(args)) {
        _log(3, "No payload provided for frame opcode: %d", frame_opcode) ;
        return 0 ;
    }

    if(!pointerp(args)) args = ({ args }) ;

    _log(3, "Preparing to format frame with opcode: %d and payload: %O", frame_opcode, args) ;

    frame = format_frame(frame_opcode, args...) ;
    written = sizeof(frame) ;
    server["sent_total"] += written ;
    result = socket_write(fd, frame) ;

    _log(3, "Frame type: %d, Message: %O", frame_opcode, args) ;
    _log(3, "Binary frame: %O", binary_to_hex(frame)) ;

    unformatted_frame = unformat_frame(frame) ;
    _log(3, "Unformatted frame: %s", unformatted_frame) ;

    if(result != EESUCCESS) {
        _log(2, "Failed to send message: %s", socket_error(result)) ;
        call_if(this_object(), "websocket_handle_message_error", result) ;
        shutdown_websocket() ;
        return 0 ;
    } else {
        _log(2, "Sent message to %s", server["request"]["host"]) ;

        call_if(this_object(), "websocket_handle_message_sent", frame_opcode, args) ;

        return 1 ;
    }
}

/**
 * Sends a ping frame to the WebSocket server.
 */
protected nomask void send_ping() {
    int result ;

    _log(2, "Sending ping frame") ;

    if(!server)
        return ;

    if(server["state"] != WS_STATE_CONNECTED)
        return ;

    result = websocket_message(WS_PING_FRAME, "") ;
    if(!result) {
        _log(2, "Failed to send ping: %d", result) ;
    } else {
        _log(2, "Sent ping to ", server["request"]["host"]) ;
    }
}

/**
 * Sends a pong frame to the WebSocket server.
 *
 * @param {string} payload - The payload for the pong frame.
 */
protected nomask void send_pong(string payload) {
    int result ;

    _log(2, "Sending pong frame") ;

    if(!server)
        return ;

    result = websocket_message(WS_PONG_FRAME, payload) ;
    if(!result) {
        _log(2, "Failed to send pong: %d", result) ;
    } else {
        _log(2, "Sent pong to %s", server["request"]["host"]) ;
    }
}

/**
 * Closes the WebSocket connection with the specified code and reason.
 *
 * @param {int} code - The close code.
 * @param {string} reason - The reason for closing the connection.
 * @returns {int} - 1 if the connection was closed successfully, 0 otherwise.
 */
varargs protected nomask int websocket_close(int code: (: WS_CLOSE_NORMAL :), string reason: (: null :)) {
    switch(code) {
        case WS_CLOSE_NORMAL:
            reason = reason || "Disconnecting" ;
            break ;
        case WS_CLOSE_GOING_AWAY:
            reason = reason || "Going away" ;
            break ;
        case WS_CLOSE_PROTOCOL_ERROR:
            reason = reason || "Protocol error" ;
            break ;
        case WS_CLOSE_UNSUPPORTED_DATA:
            reason = reason || "Unsupported data" ;
            break ;
        case WS_CLOSE_NO_STATUS_RECEIVED:
            reason = reason || "No status received" ;
            break ;
        case WS_CLOSE_ABNORMAL:
            reason = reason || "Abnormal closure" ;
            break ;
        case WS_CLOSE_INVALID_FRAME_PAYLOAD_DATA:
            reason = reason || "Invalid frame payload data" ;
            break ;
        case WS_CLOSE_POLICY_VIOLATION:
            reason = reason || "Policy violation" ;
            break ;
        case WS_CLOSE_MESSAGE_TOO_BIG:
            reason = reason || "Message too big" ;
            break ;
        case WS_CLOSE_MANDATORY_EXTENSION:
            reason = reason || "Mandatory extension" ;
            break ;
        case WS_CLOSE_INTERNAL_SERVER_ERROR:
            reason = reason || "Internal server error" ;
            break ;
        case WS_CLOSE_TLS_HANDSHAKE:
            reason = reason || "TLS handshake" ;
            break ;
        default:
            _log(2, "Unknown close code: %d", code) ;
            return ;
    }

    return websocket_message(WS_CLOSE_FRAME, code, reason) ;
}

/**
 * Generates a random string of the specified length.
 *
 * @param {int} length - The length of the string to generate.
 * @returns {string} - A random string of the specified length.
 */
protected nomask string random_string(int length) {
    string result = "" ;
    while(length--)
        result += element_of(charset) ;
    return result ;
}

/**
 * Shuts down the WebSocket connection gracefully.
 */
void shutdown() {
    websocket_close() ;
}

void mudlib_unsetup() {
    shutdown(WS_CLOSE_GOING_AWAY) ;
}
