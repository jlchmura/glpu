// /std/daemon/virtual_server.c
//
// Created:     2024/02/04: Gesslar
// Last Change: 2024/02/04: Gesslar
//
// 2024/02/04: Gesslar - Created

inherit STD_DAEMON ;

protected object generate_object(string path) ;

object compile_object(string path) {
    string subzone, subzone_path ;
    /** @type {STD_VIRTUAL_SERVER} */
    object subzone_daemon ;
    string *current_dir ;
    string *path_parts ;

    path_parts = explode(path, "/") ;
    if(sizeof(path_parts) < 1) return 0 ;

    current_dir = dir_file(this_object()) ;
    subzone = path_parts[0] ;
    subzone_path = current_dir[0] + subzone + "/zone" ;

    if(file_size(subzone_path + ".c") > 0) {
        subzone_daemon = load_object(subzone_path) ;
        if(!subzone_daemon) {
            return 0 ;
        }
        return subzone_daemon->compile_object(implode(path_parts[1..], "/")) ;
    }

    // No subzone found, handle the request
    return generate_object(path) ;
}

// To be overridden by specific zone daemons
protected object generate_object(string path) {
    return 0;  // Default behavior: don't generate anything
}
