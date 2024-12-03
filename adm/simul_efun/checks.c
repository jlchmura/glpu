/**
 * Various object checks and casts
 */

/**
 * Checks if an object is a player
 * @param {STD_PLAYER} ob - The object to check.
 * @returns {ob is STD_PLAYER} 1 if the object is a player object, otherwise 0.
 */
 int playerp(object ob) { 
    // TODO: is_player is reference in d/gmcp.c, but doesn't exist?
    return userp(ob) && ob->is_player(); 
}

/**
 * Casts an object as a player object for language server purposes
 * @param ob - The object to cast as a player object.
 * @returns {STD_PLAYER} 
 */
object as_player(object ob) {
    return ob;
}

/**
 * Checks if an object is a body object.
 * @param ob - The object to check.
 * @returns {ob is STD_BODY} 1 if the object is a body object, otherwise 0.
 */
int bodyp(object ob) {
    return objectp(ob);
}

/**
 * Checks if an object is a living object.
 * @param ob - The object to check.
 * @returns {ob is STD_LIVING} 1 if the object is a living object, otherwise 0.
 */
int livingp(object ob) {
    return objectp(ob);
}

/**
 * Casts an object as a living object;
 * @param ob - The object to cast as a living object.
 * @returns {STD_LIVING}  
 */
object as_living(object ob) {
    return ob;
}

/**
 * 
 * @param ob - The object to check.
 * @returns {ob is STD_OBJECT} 1 if the object is a valid object, otherwise 0.
 */
int objectp(object ob) {
    return efun::objectp(ob);
}

/**
 * 
 * @param {STD_ROOM} ob - The object to check.
 * @returns {ob is STD_ROOM} 1 if the object is a room object, otherwise 0. 
 */
int roomp(object ob) {
    return ob->is_room();
}

/**
 * 
 * @param ob - The object to cast as a room object.
 * @returns {STD_ROOM}
 */
object as_roomObject(object ob) {
    return ob;
}

/**
 * 
 * @param {STD_CONTAINER} ob - The object to check.
 * @returns {ob is STD_CONTAINER} 1 if the object is a container object, otherwise 0.
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
 * @returns {STD_CMD}
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

/**
 * 
 * @param {STD_WEAPON} ob 
 * @returns {ob is STD_WEAPON} 1 if the object is a weapon object, otherwise 0.
 */
int weaponp(object ob) {
    return ob->is_weapon();
}

/**
 * @param {STD_ARMOUR} ob 
 * @returns {ob is STD_ARMOUR} 1 if the object is an armour object, otherwise 0.
 */
int armourp(object ob) {
    return ob->is_armour();
}

/**
 * 
 * @param {STD_CLOTHING} ob 
 * @returns {ob is STD_CLOTHING} 1 if the object is a clothing object, otherwise 0.
 */
int clothingp(object ob) {
    return ob->is_clothing();
}

/**
 * 
 * @param ob 
 * @returns {STD_STORAGE_OBJECT} ob cast as a storage object.
 */
object as_storageObject(object ob) {
    return ob;
}

/**
 * Checks if the specified argument is a zone object.
 * @param {mixed} ob Argument to check
 * @returns {ob is STD_ZONE} 1 if the object is a zone object, otherwise 0.
 */
int zonep(mixed ob) {
    return objectp(ob) && /** @type {STD_ZONE} */(ob)->is_zone();
}