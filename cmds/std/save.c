// save.c

// Gwegster @ LPUniversity
// 30-JUNE-2005
// A simple save command

//Last edited on October 8th, 2005 by Tacitus

inherit CMD ;

int main(object caller, object room, string args)
{
  caller->save_user();
  write("Successful [save]: User saved.\n");
  return 1;
}

string help(object caller)
{
     return(HIW + " SYNTAX:" + NOR + " save\n\n" +
     "This command will save your charachter data.\n\n" +
     HIW + "See also:" + NOR + " quit\n");
}
