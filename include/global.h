#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <clean.h>
#include <daemons.h>
#include <input.h>
#include <inventory.h>
#include <messaging.h>
#include <modules.h>
#include <move.h>
#include <mudlib.h>
#include <objects.h>
#include <runtime_config.h>
#include <signal.h>

#define SIMUL_OB     "/adm/obj/simul_efun"
#define LOGIN_OB     "/adm/obj/login"
#define ROOM_VOID    "/d/std/void"
#define ROOM_FREEZER "/d/std/freezer"
#define ROOM_START   "/d/village/square"

#define LIVING_OB    "/std/living/living.c"
#define PLAYER_OB    "/std/living/player.c"
#define BODY_OB      "/std/living/body.c"
#define GHOST_OB     "/std/living/ghost.c"
#define ITEM_OB      "/std/object/item.c"
#define OBJECT_OB    "/std/object/object.c"
#define ROOM_OB      "/std/room/room.c"
#define CONTAINER_OB "/std/object/container.c"
#define COMMAND_OB   "/std/object/command.c"
#define WEAPON_OB    "/std/equip/weapon.c"
#define EQUIP_OB     "/std/equip/equip.c"

#define true 1
#define false 0
#define null ([])[0]
#define undefined ([])[0]

#define DATE "%F"
#define TIME "%T"
#define DATE_TIME "%F %T"

#endif // __GLOBAL_H__
