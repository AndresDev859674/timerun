#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <time.h>

int main(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    int year = tm_info->tm_year + 1900;

    printf("TimeRun Demo\n\n");

    if (year == 3042) {
        printf("Welcome to the year 3042!\n");
    } else {
        printf("You're not in the year 3042, come back later!\n");
        printf("   Current detected year: %d\n", year);
        printf("    HINT : try using timerun >:) ");
    }
    return 0;
}
