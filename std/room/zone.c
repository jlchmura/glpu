/**
 * @file /std/room/zone.c
 * @description Room zone module
 *
 * @created 2024/02/04 - Gesslar
 * @last_modified 2024/02/04 - Gesslar
 *
 * @history
 * 2024/02/04 - Gesslar - Created
 */


string find_path(string path) ;

/** @type {STD_ZONE} */
private nosave object zone ;

void set_zone(mixed z) {
  if(stringp(z)) {
    z = find_path(z) ;
    z = load_object(z) ;
  }

  if(!objectp(z))
    error("Invalid zone object: " + z) ;

  if(!zonep(z))
    error("Invalid zone object: " + z) ;

  zone = z ;
  zone->add_room(this_object()) ;
}

string query_zone_name() {
  if(!zonep(zone))
    return "Unknown" ;

  return zone->query_zone_name() ;
}

object query_zone() {
  return zone ;
}
