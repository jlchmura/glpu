/**
 * @file /std/daemon/discord_bot.c
 * @description Discord server daemon
 *
 * @created 2024-07-06 - Gesslar
 * @last_modified 2024-07-06 - Gesslar
 *
 * @history
 * 2024-07-06 - Gesslar - Created
 */

#include <discord_bot.h>

inherit STD_WS_CLIENT ;

void startup() ;
public nomask int is_setup() ;
void gateway_api_callback(mapping incoming) ;

public nomask void start_bot(object bot) ;
private nomask void set_bot_name(string name) ;
public nomask string query_bot_name() ;
public varargs nomask void set_restricted_channels(mixed channels...) ;
private nomask string query_token() ;
private varargs nomask void set_intents(mixed intents...) ;
private nomask void set_token(string token) ;
public nomask int query_intents() ;
void rest_api_callback(mapping response) ;

private nomask void initiate_discord(mapping response) ;
private nomask void discord_handle_event(mapping frame_info) ;
private nomask void discord_handle_hello(mapping message) ;
private nomask void discord_handle_ready(mapping message) ;
private nomask void discord_send_identify() ;
private nomask varargs void discord_send_heartbeat(int immediate) ;
protected nomask void discord_initial_heartbeat() ;
private nomask void discord_handle_heartbeat_ack(mapping message) ;

private nomask nosave gateway_request_id = null ;
private nomask nosave mapping bot_data = null ;
private nomask nosave string *restricted_channels = ({ }) ;

void mudlib_setup() {
    string file ;

    ::mudlib_setup() ;

    set_log_level(1) ;

    file = query_file_name() ;

    bot_data = ([]) ;

    if(append(base_name(), ".c") != __FILE__) {
        mapping discord_bot_config ;

        bot_data = ([]) ;
        discord_bot_config = mud_config("DISCORD_BOTS")[file] ;

        set_bot_name(discord_bot_config["name"]) ;
        set_intents(discord_bot_config["intents"]...) ;
        set_token(discord_bot_config["token"]) ;
        set_restricted_channels(discord_bot_config["restricted_channels"]...) ;
    }
}

void start_bot() {
    int id ;
    mixed err ;

    if(!is_setup())
        error("Bot not setup") ;

    if(gateway_request_id) {
        _log(1, "Already connecting to Discord Gateway API") ;
        return ;
    }

    _log(1, "Connecting to Discord Gateway API for bot %s", query_bot_name()) ;

    err = catch(gateway_request_id = HTTPC_D->fetch(
        assemble_call_back((: gateway_api_callback :)),
        "GET",
        "https://discord.com/api/gateway",
        ([
            "Accept": "application/json",
            "Connection": "close",
        ])
    )) ;

    if(err) {
        _log(1, "Failed to connect to Discord Gateway API: %O", err) ;
        return ;
    }

    if(!gateway_request_id){
        _log(1, "Failed to connect to Discord Gateway API") ;
    }

    _log(2, "Gateway request ID: %d", gateway_request_id) ;
}

void stop_bot() { websocket_close(WS_CLOSE_NORMAL, "Bot stopped") ; }

void gateway_api_callback(mapping response) {
    mapping status ;
    int code ;
    string mess ;
    object bot ;
    mixed *cb ;

    _log(2, "Received HTTP response") ;

    if(!gateway_request_id) {
        _log(1, "No gateway request ID") ;
        return ;
    }

    _log(4, "Entire HTTP conversation: %O", response) ;

    status = response["status"] ;
    code = status["code"] ;
    mess = status["message"] ;

    if(code != 200) {
        _log(1, "Failed to connect to Discord Gateway API: %d - %s", code, mess) ;
        gateway_request_id = null ;
        return ;
    }

    // Need to do this call_out, because there is some kind of race
    // condition with sockets closing before something crucial
    // happens. If you can figure out why, I'd be super grateful
    // for a fix.
    call_out_walltime((: initiate_discord, response :), 2.0) ;
    // initiate_discord(response) ;
}

void initiate_discord(mapping response) {
    mixed err ;
    mixed payload ;
    string url, host ;
    string *matches ;

    if(!response) {
        _log(1, "No response from Discord Gateway API") ;
        return ;
    }

    _log(3, "body: %O", response["body"]) ;
    _log(3, "headers: %O", response["headers"]) ;
    _log(3, "bot: %O", response["bot"]) ;

    err = catch(payload = parse_body(response["body"], response["headers"]["content-type"])) ;
    if(err) {
        _log(1, "Failed to parse response body: %O", err) ;
        call_back(response["pending"]["callback"], ([ "fd": -1, "error": "Failed to parse response body" ])) ;
        // TODO: We can't cos we don't have the FD yet 🤷‍♂️
        shutdown_websocket() ;
        return ;
    }

    if(stringp(payload)) {
        _log(3, "Response body was not JSON") ;
        return ;
    }

    _log(3, "Payload: %O", payload) ;
    _log(2, "Sizeof payload: %d", sizeof(payload)) ;

    url = payload["url"] ;

    matches = pcre_extract(url, "^wss:\\/\\/(.*)\\/?$") ;
    if(!sizeof(matches)) {
        _log(1, "Failed to extract host and path from URL") ;
        return ;
    }

    host = matches[0] ;

    _log(2, "Discord Gateway Host found (%s)", matches[0]) ;
    _log(2, "Connecting to Discord Gateway (%s)", matches[0]) ;

    websocket_connect(payload["url"]) ;
}

void websocket_handle_connected() {
    mapping discord ;

    if(!gateway_request_id) {
        _log(1, "No gateway request ID") ;
        return ;
    }

    if(!server) {
        _log(1, "No server") ;
        return ;
    }

    _log(2, "%s Connected to Discord Gateway API", query_bot_name()) ;

    discord = ([
        "ready": 0,
        "heartbeat": null,
        "heartbeat_interval": 0,
        "heartbeat_seq": null,
        "seq": null,
    ]) ;

    server["discord"] = discord ;

    _log(2, "Handing connected event to bot %s", query_bot_name()) ;
    call_if(this_object(), "bot_handle_connected") ;
}

void websocket_handle_shutdown() {
    int hb ;
    mixed *callouts ;

    _log(3, "Handling socket shutdown.") ;

    if(!server) {
        _log(2, "Not connected to any server") ;
        return ;
    }

    if(of("discord", server) && of("heartbeat", server["discord"])) {
        if(find_call_out(hb) != -1) {
            _log(2, "Stopping heartbeat") ;
            remove_call_out(hb) ;
        }
    }

    _log(2, "Handing shutdown event to bot %s", query_bot_name()) ;
    call_if(this_object(), "bot_handle_shutdown") ;

    callouts = call_out_info() ;
    callouts = filter(callouts, (: $1[0] == this_object() :)) ;
    if(sizeof(callouts) > 0) {
        _log(2, "Removing remaining callouts") ;
        filter(callouts, (: remove_call_out($1[1]) :)) ;
    }
}

void websocket_handle_close_frame(mixed payload) {
    mapping handle ;

    _log(2, "Received close frame") ;

    if(server["discord"]["ready"]) {
        _log(2, "Bot %s has been disconnected", query_bot_name()) ;
    } else {
        _log(2, "Bot %s failed to connect", query_bot_name()) ;
    }
}

// Function to parse text frames
void websocket_handle_text_frame(mapping payload) {
    // mapping payload ;

    if(!payload) {
        _log(2, "No frame info") ;
        return ;
    }

    // payload = frame_info["payload"] ;
    if(payload["s"]) {
        _log(3, "Sequence: %d", payload["s"]) ;
        server["discord"]["seq"] = payload["s"] ;
    } else
        _log(3, "No sequence") ;

    if(mapp(payload)) {
        switch(payload["op"]) {
            case DISCORD_DISPATCH:
                discord_handle_event(payload) ;
                break ;
            case DISCORD_HEARTBEAT:
                discord_send_heartbeat(1) ;
                break ;
            case DISCORD_HELLO:
                discord_handle_hello(payload) ;
                break ;
            case DISCORD_HEARTBEAT_ACK:
                discord_handle_heartbeat_ack(payload) ;
                break ;
            default:
                _log(2, "Unknown text opcode %O", payload["op"]) ;
        }
    } else {
        _log(2, "Unhandled text frame type received %O", payload) ;
    }
}

void discord_handle_event(mapping payload) {
    string event_name = payload["t"] ;
    mixed data = payload["d"] ;
    string bot_event_name ;

    _log(2, "Received event: %s", event_name) ;

    switch(event_name) {
        case EVENT_READY:
            // This event we handle here instead of in the bot only
            // because we need to record the session_id and resume_gateway_url
            // The bot can have it after we're done.
            discord_handle_ready(data) ;
            _log(2, "Handing %s event to bot %s", event_name, query_bot_name()) ;
            call_if(this_object(), "bot_handle_ready", data) ;
            return ;
    }

    // Never call the bot's handler if the source is the bot
    if(server["discord"]["bot_info"] && data["author"]) {
        if(server["discord"]["bot_info"]["id"] == data["author"]["id"])
            return ;
    }

    if(restricted_channels && sizeof(restricted_channels)) {
        if(data["channel_id"] && member_array(data["channel_id"], restricted_channels) == -1) {
            _log(2, "Ignoring event in restricted channel %O", data["channel_id"]) ;
            return ;
        }
    }

    bot_event_name = sprintf("bot_handle_%s", lower_case(event_name)) ;

    _log(2, "Handing event %s to bot %s", event_name, query_bot_name()) ;
    call_if(this_object(), bot_event_name, data) ;
}

void discord_handle_hello(mapping payload) {
    int heartbeat_interval ;

    heartbeat_interval = payload["d"]["heartbeat_interval"] ;

    _log(2, "Received Hello with heartbeat interval (%d ms)", heartbeat_interval) ;
    server["discord"]["heartbeat_interval"] = heartbeat_interval / 1000.0 ;

    discord_send_identify() ;

    server["discord"]["heartbeat"] = call_out_walltime("discord_initial_heartbeat", random(100000) / 100000.0) ;
}

void discord_send_identify() {
    buffer frame ;
    int result ;
    string identify_payload ;
    int intents ;
    string token ;

    if(!stringp(token = query_token())) {
        _log(2, "No bot token for fd") ;
        shutdown_websocket(WS_CLOSE_INTERNAL_SERVER_ERROR, "No bot token for fd") ;
        return ;
    }

    if(nullp(intents = query_intents())) {
        _log(2, "No bot intents for fd") ;
        shutdown_websocket(WS_CLOSE_INTERNAL_SERVER_ERROR, "No bot intents for fd") ;
        return ;
    }

    identify_payload = json_encode(([
        "op": 2,
        "d": ([
            "token": token,
            "intents": intents,
            "properties": ([
                "os": "linux", 
                "browser": get_config(__MUD_NAME__),
                "device": "FluffOS"
            ])
        ])
    ])) ;
 
    _log(2, "Sending Identify") ;

    result = websocket_message(WS_TEXT_FRAME, identify_payload) ;

    if(!result) {
        _log(2, "Failed to send Identify payload") ;
    } else {
        _log(2, "Identify payload sent successfully") ;
    }
}

varargs void discord_send_heartbeat(int immediate) {
    string payload ;
    int seq ;
    int result ;

    _log(2, "Sending heartbeat") ;

    seq = server["discord"]["seq"] || 0 ;

    payload = json_encode(([
        "op": DISCORD_HEARTBEAT,
        "d": seq,
    ])) ;

    result = websocket_message(WS_TEXT_FRAME, payload) ;
    if(!result) {
        _log(2, "Failed to send heartbeat") ;
    } else {
        _log(2, "Sent heartbeat to %s", query_bot_name()) ;
    }

    if(!immediate) {
        // Reschedule the next heartbeat
        server["discord"]["heartbeat"] = call_out_walltime(
            "discord_send_heartbeat",
            server["discord"]["heartbeat_interval"]
        ) ;
    }
}

protected nomask void discord_initial_heartbeat() {
    _log(2, "Sending initial heartbeat") ;

    discord_send_heartbeat() ;
    server["discord"]["heartbeat"] = null ;
}

void discord_handle_heartbeat_ack(mapping payload) {
    int seq ;

    _log(2, "Received Heartbeat ACK") ;

    seq = payload["d"] ;
    server["discord"]["seq"] = seq ;

    if(!server["discord"]["heartbeat"]) {
        _log(2, "Starting heartbeat") ;
        server["discord"]["heartbeat"] = call_out_walltime(
            "discord_send_heartbeat",
            server["discord"]["heartbeat_interval"]
        ) ;
    }
}

// Process the ready frame
private nomask void discord_handle_ready(mapping data) {
    mapping d ;
    mixed *matches ;
    string url ;
    mapping application ;

    if(!mapp(data)) {
        _log(2, "Received invalid ready frame") ;
        shutdown_websocket(WS_CLOSE_INVALID_FRAME_PAYLOAD_DATA, "Invalid ready frame") ;
        return ;
    }

    _log(2, "Received valid ready frame") ;

    server["discord"]["session_id"] = data["session_id"] ;

    url = data["resumed_gateway_url"] ;
    if(url) {
        matches = pcre_extract(url, "^wss:\\/\\/(.*)\\/?$") ;
        if(sizeof(matches)) {
            server["discord"]["resume_host"] = matches[0] ;
        }
    }

    server["discord"]["ready"] = 1 ;
    server["discord"]["bot_info"] = data["user"] ;

    _log(2, "Recorded session_id and resume_gateway_url") ;
}

/**
 * The following functions support the REST API calls to Discord
 */

nomask void discord_send_text(string event_name, mapping info) {
    mapping payload ;
    int type ;
    string url, path, endpoint ;
    string method ;
    string body ;
    string dest_id ;
    mapping request ;
    mapping headers ;
    mixed *callback ;
    mixed err ;

    if(!mapp(info))
        error("Invalid argument 1 to discord_send_text()") ;

    if(!server) {
        _log(2, "Bot not connected") ;
        return ;
    }

    if(!is_setup()) {
        _log(2, "Bot not setup") ;
        return ;
    }

    if(nullp(info["channel_id"]) || nullp(info["content"]))
        error("Invalid message to discord_send_text()") ;

    switch(event_name) {
        case EVENT_MESSAGE_CREATE:
            method = DISCORD_MESSAGE_TYPES[event_name]["method"] ;
            endpoint = DISCORD_MESSAGE_TYPES[event_name]["endpoint"] ;
            dest_id = info["channel_id"] ;
            url = sprintf("%s%s", DISCORD_ENDPOINT, sprintf(endpoint, dest_id)) ;
            body = json_encode(info) ;
            break ;
        default:
            _log(2, "Unknown message type %O", type) ;
            return ;
    }

    headers = ([
        "Authorization" : sprintf("Bot %s", query_token()),
        "Content-Type" : "application/json",
    ]) ;

    err = catch(callback = assemble_call_back((: rest_api_callback :))) ;
    if(err) {
        _log(2, "Failed to assemble callback: %O", err) ;
        return ;
    }

    _log(2, "Sending message request: body %O", body) ;

    request = HTTPC_D->fetch(callback, method, url, headers, body) ;

    _log(2, "Request sent: %O", request) ;
}

void rest_api_callback(mapping response) {
    _log(2, "Received REST API callback: %d %s", response["status"]["code"], response["status"]["message"]) ;
    _log(3, "REST API callback: %O", response) ;
}

/**
 *  The following functions are support functions for the bot
 */

private nomask void set_bot_name(string name) {
    if(!stringp(name)) error("Invalid argument 1 to set_bot_name()") ;

    set_log_prefix("("+name+")") ;

    _log(3, "Setting bot name to %O", name) ;

    bot_data["name"] = name ;
}

public nomask string query_bot_name() {
    return bot_data["name"] ;
}

private nomask string query_token() {
    object ob ;

    _log(4, "query_token() called from %O", previous_object()) ;

    if(origin() != ORIGIN_LOCAL) {
        _log(2, "Illegal call to query_token() from %O", previous_object()) ;
        return ;
    }

    return bot_data["token"] ;
}

private nomask void set_token(string token) {
    if(!stringp(token)) error("Invalid argument 1 to set_token()") ;

    _log(3, "Setting token to %O", token) ;

    bot_data["token"] = token ;
}

varargs private nomask void set_intents(mixed intents...) {
    string intent_string ;
    int intent_value ;

    if(!stringp(bot_data["name"])) error("Bot name must be set before intents") ;

    _log(3, "Setting intents to %O", intents) ;

    intents = filter(intents, (: stringp :)) ;
    intents = filter(intents, (: !nullp(GATEWAY_INTENT_BITS[$1]) :) ) ;

    if(!sizeof(intents)) {
        _log(2, "No valid intents to set") ;
        return ;
    }

    foreach(intent_string in intents) {
        intent_value |= GATEWAY_INTENT_BITS[intent_string] ;
    }

    bot_data["intents"] = intent_value ;
}

public nomask int query_intents() {
    return bot_data["intents"] ;
}

public varargs nomask void set_restricted_channels(mixed channels...) {
    if(!pointerp(channels)) {
        _log(2, "Invalid argument 1 to restrict_channels()") ;
        return ;
    }

    channels = filter(channels, (: stringp :)) ;
    if(!sizeof(channels)) {
        _log(2, "No valid channels to restrict") ;
        return ;
    }

    restricted_channels = channels ;
}

public string *query_restricted_channels() {
    return restricted_channels ;
}

public nomask int is_setup() {
    return !nullp(bot_data) &&
           stringp(bot_data["token"]) &&
           stringp(bot_data["name"]) &&
           intp(bot_data["intents"]) ;
}

void mudlib_unsetup() {
    if(server) {
        if(server["discord"] && server["discord"]["heartbeat"]) {
            remove_call_out(server["discord"]["heartbeat"]) ;
        }
    }
}
