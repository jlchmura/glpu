/**
 * @file /std/living/living.c
 * @description Setup and adjustments for living values of living beings.
 *
 * @created 2024-07-30 - Gesslar
 * @last_modified 2024-07-30 - Gesslar
 *
 * @history
 * 2024-07-30 - Gesslar - Created
 */
 
#include <living.h>
#include <attributes.h>
#include <boon.h>
#include <skills.h>
#include <vitals.h>
#include <wealth.h> 
#include <env.h>
   
public nomask void init_living() {    
    init_attributes() ;
    init_vitals() ;
    init_boon() ;
    init_wealth() ;
    init_env();     
}
