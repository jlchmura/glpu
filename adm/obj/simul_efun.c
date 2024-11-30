//simul_efun.c

//Tacitus @ LPUniversity
//02-APR-05
//Simul-efun object

// Last Change: 2022/08/23: Gesslar
// Order does not matter.
// 1. When adding new simul_efuns, prototype them in /adm/obj/simul_efun.h
// 2. When adding new files to the simuL_efun include list, in that file,
//    #include <simul_efun.h>
 
#include "/adm/simul_efun/arrays.c"
#include "/adm/simul_efun/base64.c" 
#include "/adm/simul_efun/data.c" 
#include "/adm/simul_efun/description.c"
#include "/adm/simul_efun/directory.c"
#include "/adm/simul_efun/english.c" 
#include "/adm/simul_efun/exists.c"
#include "/adm/simul_efun/file.c"
#include "/adm/simul_efun/function.c"
#include "/adm/simul_efun/grammar.c"
#include "/adm/simul_efun/identify.c"
#include "/adm/simul_efun/json.c"
#include "/adm/simul_efun/mappings.c"
#include "/adm/simul_efun/messaging.c"
#include "/adm/simul_efun/numbers.c"
#include "/adm/simul_efun/object.c"
#include "/adm/simul_efun/overrides.c"
#include "/adm/simul_efun/prompt.c"
#include "/adm/simul_efun/random.c"
#include "/adm/simul_efun/resolve_path.c"
#include "/adm/simul_efun/save.c"
#include "/adm/simul_efun/security.c"
#include "/adm/simul_efun/signal.c"
#include "/adm/simul_efun/socket.c"
#include "/adm/simul_efun/string.c"
#include "/adm/simul_efun/system.c"
#include "/adm/simul_efun/time.c"
#include "/adm/simul_efun/user.c"
#include "/adm/simul_efun/util.c"

/**
 * Checks if an object is a player
 * @param {PLAYER_OB} ob - The object to check.
 * @returns {ob is PLAYER_OB} 1 if the object is a player object, otherwise 0.
 */
int playerp(object ob) { 
    // TODO: is_player is reference in d/gmcp.c, but doesn't exist?
    return userp(ob) && ob->is_player(); 
}

/**
 * Casts an object as a player object for language server purposes
 * @param ob - The object to cast as a player object.
 * @returns {PLAYER_OB} 
 */
object as_player(object ob) {
    return ob;
}

/**
 * Checks if an object is a body object.
 * @param ob - The object to check.
 * @returns {ob is BODY_OB} 1 if the object is a body object, otherwise 0.
 */
int bodyp(object ob) {
    return objectp(ob);
}

/**
 * Checks if an object is a living object.
 * @param ob - The object to check.
 * @returns {ob is LIVING_OB} 1 if the object is a living object, otherwise 0.
 */
int livingp(object ob) {
    return objectp(ob);
}

/**
 * Casts an object as a living object;
 * @param ob - The object to cast as a living object.
 * @returns {LIVING_OB}  
 */
object as_living(object ob) {
    return ob;
}

/**
 * 
 * @param ob - The object to check.
 * @returns {ob is OBJECT_OB} 1 if the object is a valid object, otherwise 0.
 */
int objectp(object ob) {
    return efun::objectp(ob);
}

/**
 * 
 * @param {ROOM_OB} ob - The object to check.
 * @returns {ob is ROOM_OB} 1 if the object is a room object, otherwise 0. 
 */
int roomp(object ob) {
    return ob->is_room();
}

/**
 * 
 * @param ob - The object to cast as a room object.
 * @returns {ROOM_OB}
 */
object as_roomObject(object ob) {
    return ob;
}

/**
 * 
 * @param {CONTAINER_OB} ob - The object to check.
 * @returns {ob is CONTAINER_OB} 1 if the object is a container object, otherwise 0.
 */
int containerp(object ob) {
    return ob->is_container();
}

/**
 * 
 * @param ob - The object to cast as a weight object.
 * @returns {"std/object/weight.c"}
 */
object as_weightObject(object ob) {
    return ob;
}

/**
 * 
 * @param ob - The object to cast as a command object.
 * @returns {COMMAND_OB}
 */
object as_commandObject(object ob) {
    return ob;
}

/**
 * 
 * @param {"std/modules/proc.c"} ob 
 * @returns {ob is "std/modules/proc.c"} The name of the proc that can occur, or false if no proc can occur.
 */
string procp(object ob) {
    return ob->can_proc();
}