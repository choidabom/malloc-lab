// n개의 ASCII 정수 리스트를 라인마다 한 개의 정수씩 stdin에서 C 배열로 읽는 C 프로그램 작성
// 배열을 정해진 최대 배열 크기를 갖는 정적 배열로 정의
// 배열을 정해진 크기를 사용해서 할당하는 것 => 종종 나쁜 방법이 됨

#include <stdio.h>
#define MAXN 15213

int array[MAXN];

int main(){
    int n;
    scanf("%d", &n);
    if (n > MAXN)
        printf("Input file too big");
    for(int i=0; i< n; i++)
        scanf("%d", &array[i]);
    return 0;
}