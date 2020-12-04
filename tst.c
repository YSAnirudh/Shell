#include <stdio.h>
#include <unistd.h>
int main() {
	if (chdir("/mnt/c/Users/ysani/OneDrive/Desktop") != 0) {
		fprintf(stderr, "Cannot change to Directory\n");
	}
	char currDir[100];
	if (getcwd(currDir, sizeof(currDir)) == NULL) {
		printf("Error getting directory\n");
	}
	printf("%s\n", currDir);
	return 0;
}