#ifndef __LIVING_H__
#define __LIVING_H__

public nomask void init_living() ;
public nomask void adjust_living() ;
public nomask void living_adjust_attributes() ;
public nomask void living_adjust_vitals() ;

int query_peaceful(object ob);
int query_no_combat(object ob);

#endif // __LIVING_H__
