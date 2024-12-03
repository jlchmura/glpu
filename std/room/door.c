/**
 * @file /std/room/door.c
 * @description Doors are properties that can be opened and closed.
 *
 * @created 2024-09-13 - Gesslar
 * @last_modified 2024-09-13 - Gesslar
 *
 * @history
 * 2024-09-13 - Gesslar - Created
 */

#include <classes.h>
#include <exits.h>
#include <door.h>
#include <gmcp_defines.h>

inherit CLASS_DOOR ;

private nosave mapping _doors = ([ ]) ;

int add_door(class Door door) {
  // Absolutely necessary in order to be able to add a door to a room.
  if(!stringp(door.direction))
    return null ;

  if(!valid_exit(door.direction))
    return null ;

  if(!nullp(_doors[door.direction]))
    return null ;

  // Fill in missing values.
  if(nullp(door.id))
    door.id = ({sprintf("%s door", door.direction)}) ;
  if(!pointerp(door.id))
    door.id = ({door.id}) ;
  door.id = distinct_array(door.id) ;

  if(!door.status)
    door.status = "closed" ;
  if(!door.type)
    door.type = "door" ;
  if(!door.name)
    door.name = sprintf("%s %s", door.direction, door.type) ;

  door.id += ({ door.name, door.type, lower_case(door.short) }) ;

  _doors[door.direction] = door ;

  GMCP_D->broadcast_gmcp(this_object(), GMCP_PKG_ROOM_INFO, this_object()) ;

  return 1 ;
}

int valid_door(string direction) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  return 1 ;
}

int remove_door(string direction) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  map_delete(_doors, direction) ;

  GMCP_D->broadcast_gmcp(this_object(), GMCP_PKG_ROOM_INFO, this_object()) ;

  return 1 ;
}

varargs int set_door_open(string direction, int bool, int silent) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;
  if(nullp(bool))
    return null ;

  bool = !!bool ;
  door.status = bool ? "open" : "closed" ;

  _doors[direction] = door ;

  if(!silent)
    tell_down(this_object(),
      sprintf("The %s is now %s.\n", door.name, door.status)) ;

  GMCP_D->broadcast_gmcp(this_object(), GMCP_PKG_ROOM_INFO, this_object()) ;
  return bool ;
}

int query_door_open(string direction) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  return door.status == "open" ;
}

int set_door_locked(string direction, int bool, int silent) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;
  if(nullp(bool))
    return null ;
  if(door.status == "open")
    return 0 ;

  if(nullp(silent))
    silent = 0 ;
  silent = !!silent ;

  bool = !!bool ;
  door.status = bool ? "locked" : "closed" ;

  if(!silent)
    tell_down(this_object(),
      sprintf("There is a click from the %s.\n", door.name)) ;

  _doors[direction] = door ;

  GMCP_D->broadcast_gmcp(this_object(), GMCP_PKG_ROOM_INFO, this_object()) ;

  return bool ;
}

int query_door_locked(string direction) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  return door.status == "locked" ;
}

int add_door_id(string direction, string id) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  door.id += ({id}) ;
  door.id = distinct_array(door.id) ;

  _doors[direction] = door ;

  return 1 ;
}

int remove_door_id(string direction, string id) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  door.id = remove_array_element(door.id, id) ;

  _doors[direction] = door ;

  return 1 ;
}

string *query_door_ids() {
  return keys(_doors) ;
}

int set_door_short(string direction, string short) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  door.short = short ;

  _doors[direction] = door ;

  return 1 ;
}

string query_door_short(string direction) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  return door.short ;
}

int set_door_long(string direction, string long) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  door.long = long ;

  _doors[direction] = door ;

  return 1 ;
}

string query_door_long(string direction) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  return door.long ;
}

mapping query_doors() {
  return copy(_doors) ;
}

string query_door_name(string direction) {
  class Door door ;
  if(nullp(door = _doors[direction]))
    return null ;

  return door.name ;
}

string *id_door(string id) {
  string *doors = ({ }) ;

  foreach(string dir, class Door door in _doors)
    if(of(id, door.id))
      doors += ({ dir }) ;

  return doors ;
}

void reset_doors() {
  string *exits = query_exit_ids() ;

  foreach(string dir, class Door door in _doors) {
    string other_room_file ;

    // Sanity check.
    if(member_array(dir, exits) == -1) {
      remove_door(dir) ;
      continue ;
    }

    if(other_room_file = query_exit(dir)) {      
      object other_room ;

      other_room = find_object(other_room_file) ;

      if(!roomp(other_room))
        continue ;

      foreach(string other_dir in other_room->query_exit_ids()) {
        if(other_room->query_exit_dest(other_dir, true) == this_object()) {
          if(other_room->query_door_open(other_dir)) {
            set_door_open(dir, true, true) ;
            break ;
          } else if(other_room->query_door_locked(other_dir)) {
            set_door_locked(dir, true, true) ;
            break ;
          }
        }
      }
    }
  }
}

varargs mixed query_door_status(string direction, int as_number) {
  class Door door ;

  if(nullp(door = _doors[direction]))
    return null ;

  if(as_number) {
    switch(door.status) {
      case "open": return 1 ;
      case "closed": return 2 ;
      case "locked": return 3 ;
      default: return 0 ;
    }
  }

  return door.status ;
}
