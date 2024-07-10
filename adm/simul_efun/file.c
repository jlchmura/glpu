#include "/adm/obj/simul_efun.h"

#include <config.h>

/**
 * @simul_efun assure_file
 * @description Given a file, ensures that the directory structure leading to
 *              the file exists, creating directories as needed.
 * @param {string} file - The path of the file to ensure.
 */
void assure_file(string file) {
    string *path;
    string dir;
    int i;

    if(file_size(file) != -1) return;

    path = explode(file, "/");
    dir = "";

    for(i = 0; i < sizeof(path) - 1; i++) {
        dir += "/" + path[i];
        if(file_size(dir) == -1) mkdir(dir);
    }
}

//file_owner(string file);

//Tacitus @ LPUniversity
//02-APR-05
//Simul-Efun

//Last edited on July 1st, 2005 by Tacitus

/**
 * @simul_efun file_owner
 * @description Determines the owner of a file based on its path.
 * @param {string} file - The path of the file to check.
 * @returns {string} The owner of the file, or 0 if not found.
 */
string file_owner(string file) {
    string temp;

    if(file[0] != '/') file = "/" + file;

    if(sscanf(file, "/home/%s/%s/%*s", temp, temp) == 2) {
        return temp;
    }
    if(sscanf(file, "/adm/%s/%*s", temp) == 2) {
        return "Admin/" + temp;
    }

    return 0;
}

/**
 * @simul_efun tail
 * @description Returns the last few lines of a file, similar to the Unix
 *              `tail` command.
 * @param {string} path - The path of the file to read.
 * @param {int} [line_count=25] - The number of lines to read from the end of
 *                                the file. Defaults to 25.
 * @returns {string} The last few lines of the file.
 */
varargs string tail(string path, int line_count) {
    int chunk_size = 80 * 5; // Chunk size for each read operation
    string result = ""; // Accumulator for the result
    int total_lines = 0; // Counter for total lines found
    int file_size ; // Total size of the file
    int start, end; // Variables to define the read range
    int read_chars = 0; // Counter for characters read without finding a newline
    int newlines_found = 0; // Counter for newlines found in the current chunk

    if(nullp(path)) error("No file specified for tail(). [" + previous_object() + "]");
    if(nullp(line_count)) line_count = 25 ; // Default to 25 lines if not specified

    // Get the total size of the file
    file_size = file_size(path);
    // Initialize reading position at the end of the file
    end = file_size;

    // Ensure we don't start reading beyond the start of the file
    if (end < 0) return "File does not exist or is empty.";

    while (total_lines < line_count && end > 0) {
        string chunk; // Variable to hold the data read in each chunk
        start = end - chunk_size;
        if (start < 0) start = 0; // Adjust start to not go below file beginning

        // Read the chunk from the file
        chunk = read_bytes(path, start, end - start);

        if (!strlen(chunk)) break; // Break if no data was read

        // Iterate through the chunk to count newlines
        for (int i = strlen(chunk) - 1; i >= 0; i--) {
            if (chunk[i] == '\n') {
                newlines_found++;
                if (newlines_found >= line_count) break; // Stop if we have enough lines
            }
            read_chars++;

            // Check if we've read too many characters without a newline
            if (read_chars > 2000) break;
        }

        // Prepend the current chunk to the result
        result = chunk + result;

        // Adjust counters based on what was found
        total_lines += newlines_found;
        end = start; // Move the end position for the next read

        // Reset counters for the next iteration
        read_chars = 0;
        newlines_found = 0;

        // Break early if we've read too many characters without finding a newline
        if (read_chars > 2000) break;
    }

    // Trim the result if we've collected more lines than needed
    if (total_lines > line_count) {
        int lines_to_trim = total_lines - line_count;
        int pos = 0;
        for (int i = 0; i < strlen(result) && lines_to_trim > 0; i++) {
            if (result[i] == '\n') {
                lines_to_trim--;
                pos = i + 1;
            }
        }
        result = result[pos..]; // Keep only the required number of lines from the end
    }

    return result;
}


//log_file(string file, string str)

//Tacitus @ LPUniversity
//15-APR-05
//Simul-efuns

/**
 * @simul_efun log_file
 * @description Writes a log message to a specified log file.
 * @param {string} file - The name of the log file.
 * @param {string} str - The log message to write.
 * @param {mixed} [arg] - Additional arguments to include in the log message.
 * @returns {int} 1 if the log message was written successfully, otherwise 0.
 */
varargs int log_file(string file, string str, mixed arg...) {
    if(!file || !str) return 0;
    master()->log_file(file, str, arg...);
    return 1;
}

/**
 * @simul_efun explode_file
 * @description Reads a file and returns its content as an array of lines,
 *              excluding comment lines and empty lines.
 * @param {string} file - The path of the file to read.
 * @returns {string[]} An array of lines from the file.
 */
string *explode_file(string file) {
    string old_privs ;

    if(!file) return ({}) ;

    old_privs = query_privs() ;
    set_privs(this_object(), query_privs(previous_object())) ;

    if(!file_exists(file)) {
        set_privs(this_object(), old_privs) ;
        return ({}) ;
    }

    catch {
        string data = read_file(file) ;
        string *lines ;

        if(!data) {
            set_privs(this_object(), old_privs) ;
            return ({}) ;
        }
        lines = explode(data, "\n") ;
        lines = filter(lines, (: $1[0] != '#' :)) ;
        lines = filter(lines, (: strlen(trim($1)) > 0 :)) ;
    } ;

    set_privs(this_object(), old_privs) ;

    return lines ;
}

/**
 * @simul_efun implode_file
 * @description Writes an array of lines to a specified file, optionally
 *              overwriting the existing content.
 * @param {string} file - The path of the file to write to.
 * @param {string[]} lines - The array of lines to write.
 * @param {int} [overwrite=0] - Whether to overwrite the existing content.
 *                              Defaults to 0 (append).
 */
varargs void implode_file(string file, string *lines, int overwrite) {
    string old_privs ;

    if(!file) return ;
    if(!lines) return ;
    if(!sizeof(lines)) return ;

    overwrite = !!overwrite ;
    old_privs = query_privs() ;
    set_privs(this_object(), query_privs(previous_object())) ;

    catch {
        write_file(file, implode(lines, "\n") + "\n", overwrite) ;
    } ;

    set_privs(this_object(), old_privs) ;
}

/**
 * @simul_efun query_file_name
 * @description Returns the name of the file corresponding to a given object.
 * @param {object} ob - The object to query the file name of.
 * @returns {string} The name of the file corresponding to the object.
 */
string query_file_name(object ob) {
    string file, *parts;
    string dir ;

    if(!objectp(ob))
        ob = previous_object() ;

    if(!objectp(ob))
        error("Bad argument 1 to query_file_name().\n") ;

    file = base_name(ob) ;
    parts = explode(file, "/");
    file = parts[<1];

    return file;
}

/**
 * @simul_efun temp_file
 * @description Generates a temporary file name based on the provided argument.
 * @param {mixed} arg - The file or object to create a temporary file for.
 * @returns {string} The path to the temporary file.
 */
varargs string temp_file(mixed arg) {
    string file ;
    string *matches ;

    if(nullp(arg))
        arg = previous_object() ;

    if(stringp(arg)) {
        file = arg ;
    } else if(objectp(arg)) {
        file = query_privs(arg) ;
    } else {
        error("Bad argument 1 to temp_file().\n") ;
    }

    matches = pcre_extract(file, "^\\[([a-zA-Z0-9_-]+)\\]$|^([a-zA-Z0-9_-]+)$") ;
    if(sizeof(matches) < 1) {
        error("Bad argument 1 to temp_file().\n") ;
    }

    if(sizeof(matches) == 1)
        file = sprintf("%s.%d", matches[0], time_ns()) ;
    else
        file = sprintf("%s.%d", matches[1], time_ns()) ;

    return file ;
}
