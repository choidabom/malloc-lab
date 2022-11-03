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

## Understanding Assignment 
http://csapp.cs.cmu.edu/3e/malloclab.pdf (번역 참고: https://prodyou.tistory.com/21)
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
여러분에게 제공된 파일 `mm.c` 는 저희 생각에 가장 간단하면서도 정확하게 작동하는 malloc 패키지를 구현합니다. 이 파일로 시작해서, 아래 시멘틱스를 준수하도록 기능을 수정하세요. (구현 과정에서 다른 private static function을 정의해도 됩니다)

- `mm_init`: `mm_malloc`, `mm_realloc`, `mm_free`를 호출하기 전에, 응용 프로그램(예를 들면 구현 과정에 쓰일 Trace-driven 드라이버 프로그램)은 초기 힙 영역 할당과 같은 필수적인 초기화를 수행하기 위해 mm_init을 호출할 겁니다. `mm_init`은 초기화에 문제가 있으면 -1을 리턴하고, 그렇지 않으면 0을 리턴해야 합니다.

- `mm_malloc`: `mm_malloc` 루틴은 최소 size 바이트로 할당된 블록 페이로드를 가리키는 포인터를 리턴합니다. (The mm malloc routine returns a pointer to an allocated block payload of at least size bytes.) 

모든 할당된 블록은 heap 영역에 있어야 하며, 다른 할당된 덩어리들과 겹치지 않아야 합니다. (The entire allocated block should lie within the heap region and should not overlap with any other allocated chunk.) 

저희는 `표준 C 라이브러리(libc)에 지원되는 malloc`과 `당신이 구현한 mm_malloc`을 비교할겁니다. (We will comparing your implementation to the version of malloc supplied in the standard C library
(libc).)

`libc malloc`이 항상 8바이트에 정렬된 페이로드 포인터를 리턴하기 때문에, `당신이 구현한 malloc`도 항상 8바이트에 정렬된 포인터를 리턴해야 합니다. (Since the libc malloc always returns payload pointers that are aligned to 8 bytes, your
malloc implementation should do likewise and always return 8-byte aligned pointers.)

- `mm_free`: mm_free 루틴은 ptr가 가리키는 블록을 free 해주고, 아무것도 리턴하지 않습니다. (The mm free routine frees the block pointed to by ptr. It returns nothing.) 

이 루틴은 전달받은 ptr가 이전에 호출한 mm_malloc 또는 mm_realloc 에 의해 리턴된 것이면서 아직 free 되지 않았을 때만 작동을 보장합니다. (This routine is only guaranteed to work when the passed pointer (ptr) was returned by an earlier call to mm_malloc or mm_realloc and has not yet been freed.)

- `mm_realloc`: `mm_realloc`은 최소 size 바이트로 할당된 구역을 가리키는 포인터를 아래의 조건에 따라 리턴합니다. 
    - 만약 ptr가 NULL이면, 이 호출은 mm_malloc(size); 와 동일합니다. 
    - 만약 size가 0이면, 이 호출은 mm_free(ptr); 와 동일합니다.
    - 만약 ptr가 NULL이 아닐 경우, 이 ptr는 이전에 실행되 mm_malloc이나 mm_realloc에 의해 리턴된 것이어야만 한다. 
    mm_realloc 은 ptr가 가리키는 메모리 블록의 사이즈를 size 바이트에 맞게 바꾸고, 새로운 블록의 주소를 리턴합니다. 새로운 블록의 주소는 — 당신의 구현 방식이나 이전 블록의 내부 단편화(fragmentation) 정도, 그리고 realloc 요청의 size에 따라서 — 예전 블록의 주소와 같을 수도 있고 다를 수도 있다는 사실을 알아두세요.
    구(old) 블록과 새(new) 블록의 size 중 작은 크기까지는, 새 메모리 블록의 내용이 구 ptr 블록의 내용과 같게 됩니다. 다른 모든 것은 초기화되지 않습니다(uninitialized). 예를 들어, 구 블록이 8바이트 크기고 새 블록이 12바이트 크기면, 새 블록의 8바이트 까지는 구 블록의 첫 8바이트와 같을 것이고 뒤의 4바이트는 초기화되지 않은 상태일 겁니다. 동일하게, 구 블록이 8바이트 크기이고 새 블록이 4바이트 크기이면 새 블록의 내용은 구 블록의 첫 4바이트와 같을 겁니다.


위의 조건들은 C언어 기본 lib의 malloc, realloc, free 조건과 동일합니다. 완전한 문서를 더 보고 싶다면, shell에 man malloc을 입력하세요.


### 5. Heap Consistency Checker (힙 일관성 검사기)

동적 메모리 할당기(Dynamic memory allocators)는 정확하고 효율적으로 프로그래밍하기가 어렵기로 악명 높습니다. 타입이 선언되지 않은 포인터의 조작이 엄청나게 많이 이루어지기 때문이죠. 여러분은 heap checker를 써서 heap을 스캔하고 일관성을 체크하는 것이 굉장히 도움된다는 사실을 알게 될 겁니다.
=> heap checker

Q. heap checker는 뭘 체크할까? 예를 들어보면,
- free 리스트에 있는 모든 블록이 free로 표시되어 있는가?
- 연결되지 못한 연속된 free 블록이 있는가?
- 모든 free 블록이 실제로 free 리스트에 있는가?
- free 리스트에 있는 포인터가 유효한 free 블록을 가리키고 있는가?
- 할당된 블록들 중 겹치는게 있는가?
- heap 블록에 있는 포인터들이 유효한 heap 주소를 가리키고 있는가?

여러분의 heap checker는 `mm.c`에 있는 `int mm_check(void)` 함수로 구성될 것입니다. 여러분이 중요하다고 생각하는 모든 불변성, 일관성 조건을 체크하죠. heap checker는 여러분의 heap이 일관적일 경우에만 0이 아닌 값을 리턴할 것입니다. 위에 열거된 내용들을 모두 체크하려고 너무 묶여있을 필요는 없습니다. 그저 `mm_check`가 fail할 때 에러 메시지를 출력하기만 하면 됩니다.


### 6. Support Routines (지원되는 루틴들)

memlib.c 패키지는 여러분의 동적 메모리 할당기를 위한 메모리 시스템을 시뮬레이션 해줍니다. 여러분은 아래의 memlib.c 함수들을 사용할 수 있습니다.
- `void *mem_sbrk(int incr)`: 0이 아닌 양의 정수인 incr의 바이트 크기에 맞게 heap을 증가시킵니다. 그리고 새로 할당된 heap 영역의 첫 번째 바이트를 가리키는 generic 포인터를 리턴합니다. 이 함수는 오직 0이 아닌 양의 정수만을 인자로 갖는다는 점을 제외하면 Unix의 sbrk 함수와 동일합니다.
- `void *mem_heap_lo(void)`: heap의 첫번째 바이트를 가리키는 generic 포인터를 리턴합니다.
- `void *mem_heap_hi(void)`: heap의 마지막 바이트를 가리키는 generic 포인터를 리턴합니다.
- `size_t mem_heapsize(void)`: heap이 현재 몇 바이트 크기인지를 리턴합니다.
- `size_t mem_pagesize(void)`: system의 page가 몇 바이트 크기인지 리턴합니다. (Linux 시스템에서는 4K)
 

### 7. The Trace-driven Driver Program 
드라이버 프로그램인 `mdriver.c`는 여러분의 `mm.c` 패키지의 정확성, 공간 활용도, 처리량을 테스트합니다. 드라이버 프로그램은 trace file들에 의해 컨트롤됩니다. 각 trace file들은 드라이버가 mm_malloc, mm_realloc, mm_free를 호출하도록 지시하는 일련의 할당(allocate), 재할당(realloc), 해방(free) 명령을 포함하고 있습니다. 드라이버와 trace file 둘 다 나중에 여러분들의 mm.c 파일에 점수를 매길 때 사용할 도구들입니다.

mdriver.c 드라이버는 아래의 커맨드라인 인자들을 받을 수 있습니다 :
    - t <tracedir> : config.h에 정의된 기본값 디렉토리 대신에 <tracedir> 디렉토리에서 기본값 trace file을 찾게 합니다.
    - f <tracefile> : 기본 trace file들 대신에 하나의 특정한 tracefile을 사용합니다.
    - h : 커맨드라인 인자들에 대한 요약을 출력합니다. (*역자 주 : help 명령어)
    - l : 여러분의 malloc 패키지에 추가로 libc malloc도 실행하여 측정합니다.
    - v : 상세(verbose) 출력. compact table에 있는 각 trace file에 대한 성능 분석을 출력합니다.
    - V : (대문자 V) 더욱 상세한 출력. 각 trace file이 처리될 때마다 추가 진단 정보를 출력합니다. 이는 malloc 패키지의 실패 원인인 trace file이 어떤 건지 알아내기 위해서 디버깅할 때 유용합니다.


### 8. Programming Rules 
- `mm.c`안에 있는 어떤 인터페이스도 수정해서는 안 됩니다.
- 메모리 관리와 관련된 그 어떤 라이브러리 콜이나 시스템 콜도 불러와서는 안 됩니다. 이 말은 malloc, calloc, free, realloc, sbrk, brk 또는 이러한 콜과 관련된 어떠한 변수도 여러분의 코드에 사용해서는 안 된다는 것을 의미합니다.
- 여러분의 `mm.c` 파일에 그 어떤 전역 또는 정적(static)복합 데이터 구조 - 배열, 구조체, 트리, 리스트와 같은 - 도 정의하는 것이 허용되지 않습니다. 그러나, integer, float, pointer와 같은 전역 스칼라 변수를 정의하는 것은 허용됩니다.
- 8바이트 경계에 정렬된 블록을 리턴하는 libc malloc 패키지와의 일관성을 위해서, 여러분의 할당기는 항상 8바이트에 정렬된 포인터를 리턴해야 합니다. 드라이버는 여러분에게 이 요구사항을 강요할 겁니다.

### 9.  Evaluation (채점 !)

### 10. Handin Instructions (생략)

### 11. Hints 
- mdriver -f 옵션을 사용하세요. 초기 개발 단계에서, 작은 trace file을 사용하는 것은 디버깅과 테스팅을 단순하게 만들 것입니다. 이와 같은 작은 trace files 두 개를 (short1,2-bal.rep) 포함해두었으니 초기 디버깅 단계에 사용하세요.
- mdriver -v 와 -V 옵션을 사용하세요. -v 옵션은 각 trace file에 대한 자세한 요약을 제공합니다. -V 옵션도 각 trace file이 읽힐 때마다 상세한 요약을 표시해주므로, 에러를 분리해내는데에 도움이 될 겁니다.
- gcc -g로 컴파일하고 디버거를 사용하세요. 디버거는 당신이 범위를 벗어난 메모리 참조(out of bounds memory references)를 분리해서 찾아내는 데 도움이 됩니다.
- 교재의 모든 malloc 구현에 관한 내용을 읽고 이해하세요. 교재는 묵시적(implicit) 가용 리스트에 기반한 간단한 할당기에 대한 상세한 예시를 제공합니다. 교재를 읽고 이해하는 것을 시작점으로 잡으세요. 이 간단한 '묵시적 리스트 할당기'에 대해 완벽하게 이해하기 전까지는 할당기 작업을 시작하지 마세요.
- 포인터 연산을 C 전처리 매크로로 캡슐화하세요. 필요한 모든 캐스팅들 때문에 메모리 관리자에서의 포인터 연산은 복잡하고 에러가 발생하기 쉽습니다. 포인터 작업에 매크로를 쓰면 복잡도를 크게 줄일 수 있습니다. 예시는 교재를 참고하세요.
- 단계적으로 구현하세요. 9개의 첫 trace들은 malloc과 free 구현을 요구합니다. 마지막 2개의 trace들은 realloc, malloc, free를 요구합니다. 먼저 첫 9개 trace들에서 정확하고 효율적인 malloc, free 루틴이 동작하게끔 하는 걸 추천합니다. 그 이후에 realloc 구현을 신경쓰세요. 초심자들은 malloc과 free 구현을 완료하고 그 위에 realloc을 빌드하세요. 하지만 나중에 정말 좋은 성능을 위해서는 realloc을 단독으로 빌드해야 할 겁니다.
- profiler를 사용하세요. 성능을 최적화하는데에 gprof 도구가 도움이 될 것입니다.
- 최대한 빨리 시작하세요! 당신은 몇 페이지의 코드만으로도 효율적인 malloc 패키지를 작성할 수 있을 겁니다. 하지만 동시에 이 코드는 여러분이 지금까지 작성한 코드들 중에 가장 어렵고 복잡한 코드 중 하나일 거에요. 그러니까 얼른 시작하시고, 행운을 빕니다!