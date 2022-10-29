// 실행 시에 메모리를 할당
// 배열의 최대 크기는 가용한 가상메모리의 양에 의해서만 제한된다.
#include <stdio.h>
#include <stdlib.h>

int main(){
    int *array, n;
    scanf("%d", &n);
    array = (int *)malloc(n*sizeof(int));

    for(int i=0; i< n; i++)
        scanf("%d", &array[i]);
    free(array);
    return 0;
}