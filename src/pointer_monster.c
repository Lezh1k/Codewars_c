#include "pointer_monster.h"

//static char a[4][7] = {"Common", "Point", "Boost", "Better"};
//static char* b[4] = {a+3, a+1, a, a+2};

//char* (*(c)(void))[4] {
//  return &b;
//}
/////////////////////////////////////////////////////////

//char* (*(d)(void))[1] {
//    return c()[1] - 3;
//}
/////////////////////////////////////////////////////////

//static char buf[256];
//char *pointer_monster(char *(*(*f)(void))[1]) {
//  int len;
//  len  = sprintf(buf, "%s", *f()[0]);
//  len += sprintf(buf + len, "%s ", *((**f)()-1)[0] + 4);
//  len += sprintf(buf + len, "%s", (*f())[0] - 4);
//  len += sprintf(buf + len, "%s", f()[0][2] + 3);
//  len += sprintf(buf + len, "%s", *((**f)()-1)[0] + 4);
//  return buf;
//}
/////////////////////////////////////////////////////
