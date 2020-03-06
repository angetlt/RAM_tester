#include <stdio.h>
#include <string.h>
int main(void)
{
char *p;
p = strtok("The summer soldier, the sunshine patriot"," ");
printf(p);
do {
p=strtok('\0', ", ");
if(p) printf("|% s", p);
} while(p);
return 0;
}