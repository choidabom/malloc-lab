# Malloc Lab
## WEEK06: 시스템 콜, 데이터 세그먼트, 메모리 단편화, sbrk/mmap
malloc을 C언어로 구현하면서, C언어 포인터의 개념, gdb 디버거 사용법 등을 익혀본다.

- '동적 메모리 할당' 방법을 직접 개발하면서 메모리, 포인터 개념에 익숙해질 수 있는 과제입니다. 랩 코드를 직접 수정하고, 채점 프로그램을 실행하면서 '내 점수'를 알 수 있습니다.
- **즉, 나만의 malloc, realloc, free 함수를 구현하는 것!**
- implicit, explicit, seglist 등 여러 방법이 있지만, 일단 `implicit` 방법부터 구현해 보겠습니다. 여유가 되면 `explicit`, `seglist`, `buddy system` 까지도 도전해보세요.
- 컴퓨터 시스템 교재의 9.9장을 찬찬히 읽어가며 진행하세요. 교재의 코드를 이해하고 옮겨써서 잘 동작하도록 실행해보는 것이 시작입니다!
- https://github.com/SWJungle/malloclab-jungle 의 내용대로 진행합니다.
    - malloc-lab 과제 설명서 - 출처: CMU (카네기멜론 대학교)
    - `mm.c`를 구현하고 `mdriver`로 채점(테스트) 합니다.
    - 진행방법
        - make 후 `./mdriver` 를 실행하면 `out of memory` 에러 발생
        - 책에 있는 implicit list 방식대로 malloc을 구현해서 해당 에러를 없애기
        - 이후 (시간이 된다면) explicit list와 seglist 등을 활용해 점수를 높이기
        - Tip: `./mdriver -f traces/binary2-bal.rep` 와 같이 특정 세트로만 채점 할 수 있다.

## GOAL
- 일단 `implicit` 방법으로 `./mdriver` 가 정상 작동하도록 코드를 완성하기
- 💡 코드에 따라 점수는 다를 수 있습니다! 그러나, 잘 이해했고, 잘 돌아가게 했으면 1차 완성🙂


<details><summary style="color:skyblue">CMU 강의자료 전체</summary>

- malloc lab과 직접 관련되는 section들
    - 19장: malloc-basic
        GNU malloc API와 다양한 구현 방식 설명
        implicit list 방식의 malloc 구현에 관한 자세한 설명
    - 20장: malloc-advanced
        - explicit & seg list방식의 malloc 구현, garbage collection 개념에 대한 설명
        - well-known memory bug에 관한 설명
        - 44p : gdb, valgrind등의 memory bug를 잡기위한 접근 방식 설명 포함
- malloc lab 수행에 도움을 줄 수 있는 내용을 담은 section들
    - 14장: ecf-procs
        - exception의 개념과 종류, process의 개념에 대한 설명
        - 14p : segmentation fault에 관한 설명 포함
    - 11장: memory-hierarchy
        - 컴퓨터 시스템의 메모리 계층구조에 대한 설명
    - 17장: vm-concepts
        - 가상 메모리와 page fault에 대한 이론적 설명

</details>

## TIL (Today I Learned)
### 10.28 금

- [메모리 영역(데이터 세그먼트) 이해](https://bo5mi.tistory.com/159)
- [가상메모리 이해](https://bo5mi.tistory.com/161)
- malloc-lab 과제 설명서 잘 읽고 이해하기 

### 10.29 토

- Do it C 메모리 할당 강의 1-2 (총 2시간 상당)
- CMU 강의자료 19장 
- CS:APP 9.9.1 ~ 9.9.12 정독 및 정리
- implicit 방법 코드 구현 

### 10.30 일

- CMU 강의자료 20장 (garbage 컬렉션 전까지)
- CS:APP 9.9.12 ~ 9.9.14 정독 및 정리
- explicit, seglist, buddy system 방법 코드 구현 

### 10.31 월
- CMU 강의 자료 4, 11, 17장

### 11.01 화
- CSAPP 7장 링커(7.1, 7.4, 7.9, 그림 7.15), 8장 예외적인 제어 흐름 (특히 8.1, 8.5)

### 11.02 수
- Self-Test 💡


## Understanding Assignment 
http://csapp.cs.cmu.edu/3e/malloclab.pdf
### 1. Introduction

이번 실습에서는 C언어로 동적 메모리 할당기(dynamic storage allocator)를 만들게 됩니다. 즉, malloc, free, realloc을 여러분 버전으로 만드는거죠. 디자인 스페이스를 창의적으로 설계해서, 정확하고 효율적이며 속도도 빠른 할당기를 구현해보세요.

### 2. Logistics

최대 두 명 정도까지 그룹지어서 작업하세요.
과제 내용에 대한 모든 정확한 설명, 수정사항 등은 코스 웹 페이지에 게시됩니다.

### 3. Hand Out Instructions (배포 파일 설명)

당신이 다뤄야 할 파일은 `mm.c`입니다.

`mdriver.c` 프로그램은 여러분이 만든 솔루션의 성능을 평가하는 드라이버 프로그램입니다.

`make` 커맨드를 사용해서 드라이버 코드를 생성하고 `./mdriver -V` 커맨드로 실행해보세요.

추후 랩을 완료하면, 솔루션이 들어 있는 파일(mm.c) 하나만 제출하게 됩니다.

### 4. How to Work on the Lab
Your dynamic storage allocator(동적 메모리 할당기) will consist of the following four functions, which are declared in mm.h and defined in mm.c (mm.h에 선언, mm.c에 정의)

```c
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
```
The mm.c file we have given you implements the simplest but still functionally correct malloc package that we could think of. Using this as a starting place, modify these functions (and possibly define other private
static functions), so that they obey the following semantics:

### 5. Heap Consistency Checker

### 6. Support Routines

### 7. The Trace-driven Driver Program

### 8. Programming Rules

### 9.  Evaluation

### 10. Handin Instructions

### 11. Hints