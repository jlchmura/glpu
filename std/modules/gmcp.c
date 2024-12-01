// /std/modules/gmcp.c
// Module inherited by living objects to handle GMCP
//
// Created:     2024/02/22: Gesslar
// Last Change: 2024/02/22: Gesslar
//
// 2024/02/22: Gesslar - Created

#include <daemons.h>
#include <classes.h>
#include <type.h>

#include "/std/living/include/env.h"

inherit CLASS_GMCP ;

// Functions
public int gmcp_enabled() ;
private mixed gmcp_stringify(mixed data) ;

// Functions from other objects
varargs void _log(mixed args...) ;

private nosave mapping gmcp_data = ([ ]) ;

// This apply receives the GMCP messages from the client. We parse out the
// information into what package, subpackage, command, and payload. We then
// call the appropriate module to handle the GMCP message.
//
// We do not check if the player has GMCP enabled here. We assume that the
// client will not send GMCP messages if it is not enabled, and, we want to
// ensure that we still get Core.Hello and Core.Supports if available during
// login, since they are most likely only sent at that time. If a module does
// respond to the client, it should route its message through the GMCP_D which
// will check if the player has GMCP enabled and behave accordingly.
void gmcp(string message) {
    class ClassGMCP gmcp ;
    mixed err ;
    string gmcp_module ;
    object ob ;

    gmcp = GMCP_D->convert_message(message) ;

    if(gmcp == null)
        return ;

    if(gmcp.package == null || gmcp.module == null)
        return ;

    gmcp_module = DIR_STD_MODULES "gmcp/" + gmcp.package + ".c" ;
    if(!file_exists(gmcp_module)) {
        log_file("system/gmcp", "[%s] Module %s not found [%O]",
            ctime(),
            gmcp_module,
            previous_object() || this_object()
        ) ;
        return ;
    }

    if(err = catch(ob = load_object(gmcp_module)))
        return ;

    if(gmcp.submodule)
        call_other(ob, gmcp.module, gmcp.submodule, gmcp.payload) ;
    else
        call_other(ob, gmcp.module, gmcp.payload) ;
}

// This function sends a GMCP message to the client. It will only send the
// message if the player has GMCP enabled. If the player does not have GMCP
// enabled, the message will be ignored.
varargs void do_gmcp(string package, mixed data) {
    string message ;

    if(!get_config(__RC_ENABLE_GMCP__))
        return ;

    if(!gmcp_enabled())
        return ;

    if(data)
        data = json_encode(gmcp_stringify(data)) ;
    else
        data = "" ;

    message = trim(sprintf("%s %s", package, data)) ;

    send_gmcp(message) ;
}

// This funcation sanitises the data to be sent to the client. It will convert
// the data to a string if it is not already a string. For mappings and arrays,
// it will convert the data to a string representation of the data. GMCP should
// always send keys and values as strings, so we convert them here.
mixed gmcp_stringify(mixed data) {
    switch(typeof(data)) {
        case T_STRING:
            return data ;
        case T_INT:
            return sprintf("%d", data) ;
        case T_ARRAY: {
                mixed *result ;

                result = map(data, (: gmcp_stringify :)) ;

                return result ;
            }
        case T_OBJECT:
            return "" ;
        case T_MAPPING: {
                mapping result = ([ ]) ;
                mixed key, value ;

                foreach(key, value in data) {
                    result[gmcp_stringify(key)] = gmcp_stringify(value) ;
                }

                return result ;
            }
        case T_FUNCTION:
            return "" ;
        case T_FLOAT:
            return sprintf("%f", data) ;
        case T_BUFFER:
            return "" ;
        case T_CLASS:
            return "" ;
        default:
            return "" ;
    }

    return "" ;
}

void clear_gmcp_data() {
    gmcp_data = ([ ]) ;
}

void set_gmcp_client(mapping data) {
    gmcp_data["client"] = data ;
}

mapping query_gmcp_client() {
    return copy(gmcp_data["client"]) ;
}

void set_gmcp_supports(mapping data) {
    gmcp_data["supports"] = data ;
}

mapping query_gmcp_supports() {
    return copy(gmcp_data["supports"]) ;
}

// Function to determine if a specific package (and optionally module/
// submodule) is supported
int query_gmcp_supported(string fullname) {
    string *parts, package, module, submodule ;
    mapping supports = query_gmcp_supports() ;
    class ClassGMCP gmcp ;

    gmcp = GMCP_D->convert_message(fullname) ;

    // Check if the package is supported
    if(!supports[package]) return 0; // Package not found

    // If a module is specified, check for its support
    if(module && supports[package]["modules"]) {
        if(!supports[package]["modules"][module]) return 0; // Module not found

        // If a submodule is specified, check for its support
        if(submodule && supports[package]["modules"][module]["submodules"]) {
            if(!supports[package]["modules"][module]["submodules"][submodule]) return 0; // Submodule not found
        } else if(submodule) {
            // Submodule specified but no submodules are supported under the module
            return 0 ;
        }
    } else if(module) {
        // Module specified but no modules are supported under the package
        return 0 ;
    }

    // If we've reached this point, the specified package (and optionally module/submodule) is supported
    return 1 ;
}


public int gmcp_enabled() {
    if(!has_gmcp(this_object()))
        return 0 ;

    if(base_name(this_object()) == LOGIN_OB)
        return 1 ;

    return query_pref("gmcp") != "off" ;
}
