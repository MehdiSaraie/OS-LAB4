#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define first argv[1]
#define second argv[2]

int main(int argc,char*argv[]){
	int file=open("strdiff_result.txt",O_CREATE|O_RDWR);
	if(argc==3){
		int i=0;
		char result[16];
		int result_size=strlen(first);

		while(i<strlen(first)){
			if(i>=strlen(second)){
				result[i]='0';
				i++;
				continue;
			}

			if(first[i]<='Z')
				first[i]+='z'-'a'+1;
			if(second[i]<='Z')
				second[i]+='z'-'a'+1;

			if(first[i]<second[i]){
				result[i]='1';
			}
			else
				result[i]='0';
			i++;
		}

		while(i<strlen(second)){
			result_size=strlen(second);
			result[i]='1';
			i++;
		}

		write(file,result,result_size);
		close(file);
	}
	exit();
}
