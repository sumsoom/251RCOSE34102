
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_P 50 // 최대 프로세스 수
#define MAX_IO 3 // 각 프로세스 당 최대 I/O 요청 횟수

/* 데이터 구조 */
typedef struct
{
    int pid;
    int arrival;              // 도착 시간
    int burst;                // 원본 CPU burst
    int remain;               // 잔여 CPU burst
    int io_count;             // 각 프로세스 당 I/O 요청 개수
    int io_req[MAX_IO];       // 각 I/O 요청 시점
    int io_burst[MAX_IO];     // 각 I/O 처리 시간
    int next_io;              // 다음 처리할 I/O 인덱스
    int io_remaining[MAX_IO]; // 각 I/O 남은 시간
    int in_io;                // 0: ready 큐, 1: I/O 큐
    int priority;
    int finish_time; // 종료 시각
} PCB;

/* ready queue: 원형 큐 */
typedef struct
{
    int index[MAX_P];
    int front, rear;
} Queue;

/* queue 함수 */
void q_init(Queue *q)
{
    q->front = q->rear = 0;
}
int q_empty(Queue *q)
{
    return q->front == q->rear;
}
void q_push(Queue *q, int idx)
{
    q->index[q->rear] = idx;
    q->rear = (q->rear + 1) % MAX_P; // 원형 큐 처리
} // 맨 뒤에 하나 넣기
int q_pop(Queue *q)
{
    int item = q->index[q->front];
    q->front = (q->front + 1) % MAX_P; // 원형 큐 처리
    return item;
} // 맨 앞에서 하나 꺼내기

/* I/O 완료 체크 */
void check_io_finish(PCB P[], int n, Queue *ioq, Queue *rq)
{
    // ioq: I/O 요청 중인 프로세스의 인덱스 저장 -> ioq에 들어있는 프로세스 수만큼 반복
    int cnt = (ioq->rear - ioq->front + MAX_P) % MAX_P;
    for (int i = 0; i < cnt; i++)
    {
        int idx = q_pop(ioq); // 프로세스 인덱스
        // 현재 처리 중인 I/O 이벤트가 끝났는지 확인 후 다음으로 이동
        if (P[idx].io_remaining[P[idx].next_io] <= 0)
        {
            P[idx].next_io++;
            P[idx].in_io = 0;
            q_push(rq, idx);
        }
        else
        {
            // 아직 I/O 버스트 중이면 ioq에 다시 넣음
            q_push(ioq, idx);
        }
    }
}

/* 매 tick 마다 io_remaining 감소 */
void tick_io(PCB P[], int n, Queue *ioq)
{
    int cnt = (ioq->rear - ioq->front + MAX_P) % MAX_P;
    for (int i = 0; i < cnt; i++)
    {
        int idx = q_pop(ioq);
        P[idx].io_remaining[P[idx].next_io]--;
        q_push(ioq, idx);
    }
}

/* arrival 체크 */
void check_arrival(PCB P[], int n, int clock, Queue *rq)
{
    for (int i = 0; i < n; i++)
    {
        if (P[i].arrival == clock)
        {
            // 프로세스 도착 시간이 현재 시각과 같으면 레디큐로 이동 -> 레디 큐를 도착 순서대로 정리
            q_push(rq, i);
        }
    }
}

/* 랜덤 프로세스 생성 */
int create_processes(PCB P[], int n)
{
    srand((unsigned)time(NULL)); // 매 실행(현재 시각 기준)마다 다른 난수 생성
    for (int i = 0; i < n; i++)
    {
        int k = 1 + rand() % MAX_IO; // 1~MAX_IO개의 랜덤 I/O 이벤트 수
        P[i].pid = i + 1;
        P[i].arrival = rand() % 10;   // 0~9 사이의 랜덤 arrival time
        P[i].burst = 3 + rand() % 15; // 3~17 사이의 랜덤 CPU burst time
        P[i].remain = P[i].burst;
        P[i].io_count = k;
        for (int j = 0; j < k; j++)
        {
            // 각 I/O 요청 시점을 burst 구간 내에서 랜덤하게 결정
            P[i].io_req[j] = 1 + rand() % (P[i].burst - 1);
            P[i].io_burst[j] = 1 + rand() % 4; // 1~4 사이의 랜덤 I/O burst time
        }
        // io_req 오름차순 정렬: 버블정렬
        for (int a = 0; a < k - 1; a++) // 한 번 돌때마다 가장 큰 값이 맨 뒤로 감감
        {
            for (int b = 0; b < k - 1 - a; b++)
            {
                if (P[i].io_req[b] > P[i].io_req[b + 1])
                {
                    // tmp 이용한 스왑
                    int tmp_req = P[i].io_req[b];
                    P[i].io_req[b] = P[i].io_req[b + 1];
                    P[i].io_req[b + 1] = tmp_req;
                    // burst time도 같이 스왑
                    int tmp_burst = P[i].io_burst[b];
                    P[i].io_burst[b] = P[i].io_burst[b + 1];
                    P[i].io_burst[b + 1] = tmp_burst;
                }
            }
        }
        // 나머지 슬롯 초기화
        for (int j = k; j < MAX_IO; j++)
        {
            P[i].io_req[j] = -1;
            P[i].io_burst[j] = 0;
        }

        P[i].priority = 1 + rand() % n; // 랜덤 우선순위: 1(높음)~n(낮음)
        P[i].finish_time = -1;
    }
    return 0;
}

/* 키보드 입력 프로세스 생성 */
int input_processes(PCB P[], int n)
{
    printf("=== Manual Process Input ===\n");
    for (int i = 0; i < n; i++)
    {
        P[i].pid = i + 1;
        printf("Process %d:\n", P[i].pid);
        printf("  Arrival time: ");
        scanf("%d", &P[i].arrival);
        printf("  CPU burst time: ");
        scanf("%d", &P[i].burst);

        // I/O 이벤트 개수 입력
        printf("  Number of I/O events (0~%d): ", MAX_IO);
        scanf("%d", &P[i].io_count);
        if (P[i].io_count < 0)
        {
            P[i].io_count = 0;
        }
        if (P[i].io_count > MAX_IO)
        {
            P[i].io_count = MAX_IO;
        }
        // 각 I/O 요청 시점, 버스트 시간 입력
        for (int j = 0; j < P[i].io_count; j++)
        {
            printf("    I/O #%d request time: ", j + 1);
            scanf("%d", &P[i].io_req[j]);
            printf("    I/O #%d burst time: ", j + 1);
            scanf("%d", &P[i].io_burst[j]);
        }
        // 입력된 이벤트를 io_req 기준으로 오름차순 정렬
        for (int a = 0; a < P[i].io_count - 1; a++)
        {
            for (int b = 0; b < P[i].io_count - 1 - a; b++)
            {
                if (P[i].io_req[b] > P[i].io_req[b + 1])
                {
                    int tmp_req = P[i].io_req[b];
                    P[i].io_req[b] = P[i].io_req[b + 1];
                    P[i].io_req[b + 1] = tmp_req;
                    int tmp_burst = P[i].io_burst[b];
                    P[i].io_burst[b] = P[i].io_burst[b + 1];
                    P[i].io_burst[b + 1] = tmp_burst;
                }
            }
        }
        // 남은 I/O 배열 초기화
        for (int j = P[i].io_count; j < MAX_IO; j++)
        {
            P[i].io_req[j] = -1;
            P[i].io_burst[j] = 0;
        }

        printf("  Priority (1=highest ~ %d=lowest): ", n);
        scanf("%d", &P[i].priority);
        if (P[i].priority < 1)
        {
            P[i].priority = 1;
        }
        if (P[i].priority > n)
        {
            P[i].priority = n;
        }

        P[i].remain = P[i].burst;
        P[i].finish_time = -1;
    }
    return 0;
}

/* 스케줄러 평가 항목 계산 */
void calc_result(const PCB P[], int n, const char *tag)
{
    double wsum = 0, tsum = 0;
    printf("\n[%s] result\nPID\tAT\tBT\tFT\tTAT\tWT\n", tag);
    for (int i = 0; i < n; i++)
    {
        int tat = P[i].finish_time - P[i].arrival; // 반환 시간 = 완료 시간 - 도착 시간
        int wt = tat - P[i].burst;                 // 대기 시간 = 반환 시간 - 실행 시간
        wsum += wt;
        tsum += tat;
        printf("%2d\t%2d\t%2d\t%2d\t%2d\t%2d\n",
               P[i].pid, P[i].arrival, P[i].burst,
               P[i].finish_time, tat, wt);
    }
    printf("Average Turnaround Time = %.2f, Average Waiting Time = %.2f\n",
           tsum / n, wsum / n);
}

/* 간트 차트 출력 */
void print_gantt(const int *timeline, int len)
{
    printf("\nGantt Chart:\n");

    // 위쪽 바
    printf(" ");
    for (int i = 0; i < len; i++)
    {
        printf("------");
    }
    printf("-\n|");

    // 프로세스 PID
    for (int i = 0; i < len; i++)
    {
        if (timeline[i] == 0)
            printf(" IDL |"); // idle 표시
        else
            printf(" P%-2d |", timeline[i]);
    }

    // 아래쪽 바
    printf("\n ");
    for (int i = 0; i < len; i++)
    {
        printf("------");
    }
    printf("-\n");

    // 시간 첨자
    printf("\n");
    printf("0");
    for (int i = 1; i <= len; i++)
    {
        // 간격 조정
        if (i < 10)
            printf("     %d", i);
        else if (i < 100)
            printf("    %d", i);
        else
            printf("   %d", i);
    }
    printf("\n\n");
}

/* --------------------- 알고리즘 ---------------------  */

/* 1) FCFS */
void fcfs(PCB *P, int n)
{
    PCB tmp[MAX_P];
    memcpy(tmp, P, n * sizeof(PCB)); // 메모리 블록을 바이트 단위로 그대로 복사하는 함수: 원본 배열 P를 임시 배열 tmp로 복사
    Queue rq, ioq;
    q_init(&rq);
    q_init(&ioq);

    // 프로세스 초기 I/O 세팅
    for (int i = 0; i < n; i++)
    {
        tmp[i].remain = tmp[i].burst;
        tmp[i].next_io = 0;
        tmp[i].in_io = 0;
        for (int j = 0; j < tmp[i].io_count; j++)
        {
            tmp[i].io_remaining[j] = tmp[i].io_burst[j];
        }
    }
    int clock = 0, done = 0, tl[MAX_P * 50], tp = 0;
    int running = -1; // 현재 실행중인 프로세스 인덱스

    while (done < n)
    {
        check_arrival(tmp, n, clock, &rq);  // 새로 도착한 프로세스 rq push
        tick_io(tmp, n, &ioq);              // I/O 큐의 io_remaining time 1감소
        check_io_finish(tmp, n, &ioq, &rq); // I/O 끝난 경우 ioq pop, rq push

        // I/O 진입 검사
        if (running >= 0)
        {
            int executed = tmp[running].burst - tmp[running].remain;
            if (tmp[running].next_io < tmp[running].io_count && executed == tmp[running].io_req[tmp[running].next_io])
            {
                tmp[running].in_io = 1;
                q_push(&ioq, running);
                running = -1; // CPU 비움
            }
        }
        if (running < 0 && !q_empty(&rq))
        {
            running = q_pop(&rq);
        }

        if (running >= 0)
        {
            tmp[running].remain--;
            tl[tp++] = tmp[running].pid;
            clock++;

            if (tmp[running].remain == 0)
            {
                tmp[running].finish_time = clock;
                done++;
                running = -1;
            }
        }
        else
        {
            tl[tp++] = 0;
            clock++;
        }
    }
    print_gantt(tl, tp);
    calc_result(tmp, n, "FCFS");
}

/* 2) SJF (비선점) */
void sjf_np(PCB *P, int n)
{
    PCB tmp[MAX_P];
    memcpy(tmp, P, n * sizeof(PCB)); // 메모리 블록을 바이트 단위로 그대로 복사하는 함수: 원본 배열 P를 임시 배열 tmp로 복사
    Queue rq, ioq;
    q_init(&rq);
    q_init(&ioq);

    // 프로세스 초기 I/O 세팅
    for (int i = 0; i < n; i++)
    {
        tmp[i].remain = tmp[i].burst;
        tmp[i].next_io = 0;
        tmp[i].in_io = 0;
        for (int j = 0; j < tmp[i].io_count; j++)
        {
            tmp[i].io_remaining[j] = tmp[i].io_burst[j];
        }
    }
    int clock = 0, done = 0, tl[MAX_P * 50], tp = 0;
    int running = -1;
    // 한 틱 단위로 한 사이클 수행하는 반복문
    while (done < n)
    {
        check_arrival(tmp, n, clock, &rq);
        tick_io(tmp, n, &ioq);
        check_io_finish(tmp, n, &ioq, &rq);

        // I/O 진입 검사
        if (running >= 0)
        {
            int executed = tmp[running].burst - tmp[running].remain;
            if (tmp[running].next_io < tmp[running].io_count && executed == tmp[running].io_req[tmp[running].next_io])
            {
                tmp[running].in_io = 1;
                q_push(&ioq, running);
                running = -1;
            }
        }
        // 현재 진행 프로세스 없을 때 SJF 기준으로 새 프로세스 선택
        if (running < 0)
        {
            // 레디 큐의 프로세스 중 burst 최소인 것 선택
            int sel = -1, minb = 1e9;
            for (int i = 0; i < n; i++)
            {
                if (tmp[i].remain > 0 && tmp[i].in_io == 0 && tmp[i].arrival <= clock)
                {
                    if (tmp[i].burst < minb)
                    {
                        minb = tmp[i].burst;
                        sel = i;
                    }
                }
            }
            running = sel;
        }
        // 1틱 실행
        if (running >= 0)
        {
            tmp[running].remain--;
            tl[tp++] = tmp[running].pid;
            clock++;
            if (tmp[running].remain == 0)
            {
                tmp[running].finish_time = clock;
                done++;
                running = -1;
            }
        }
        else
        {
            tl[tp++] = 0; // idle
            clock++;
        }
    }
    print_gantt(tl, tp);
    calc_result(tmp, n, "SJF-NP");
}

/* 3) SJF (선점) */
void sjf_p(PCB *P, int n)
{
    PCB tmp[MAX_P];
    memcpy(tmp, P, n * sizeof(PCB)); // 메모리 블록을 바이트 단위로 그대로 복사하는 함수: 원본 배열 P를 임시 배열 tmp로 복사
    Queue rq, ioq;
    q_init(&rq);
    q_init(&ioq);
    // 프로세스 초기 I/O 세팅
    for (int i = 0; i < n; i++)
    {
        tmp[i].remain = tmp[i].burst;
        tmp[i].next_io = 0;
        tmp[i].in_io = 0;
        for (int j = 0; j < tmp[i].io_count; j++)
        {
            tmp[i].io_remaining[j] = tmp[i].io_burst[j];
        }
    }
    int done = 0, clock = 0, tl[MAX_P * 50], tp = 0;
    int running = -1;
    // 한 틱 단위로 한 사이클 수행하는 반복문
    while (done < n)
    {
        check_arrival(tmp, n, clock, &rq);
        tick_io(tmp, n, &ioq);
        check_io_finish(tmp, n, &ioq, &rq);

        // I/O 진입 검사
        if (running >= 0)
        {
            int executed = tmp[running].burst - tmp[running].remain;
            if (tmp[running].next_io < tmp[running].io_count && executed == tmp[running].io_req[tmp[running].next_io])
            {
                tmp[running].in_io = 1;
                q_push(&ioq, running);
                running = -1;
            }
        }
        // 매 틱마다 선점형 SJF 기준으로 프로세스 선택
        // 레디 큐의 프로세스 중 remain 최소인 것 선택
        int sel = -1, minr = 1e9;
        for (int i = 0; i < n; i++)
        {
            if (tmp[i].remain > 0 && tmp[i].in_io == 0 && tmp[i].arrival <= clock)
            {
                if (tmp[i].remain < minr)
                {
                    minr = tmp[i].remain;
                    sel = i;
                }
            }
        }
        running = sel;

        // 1틱 실행
        if (running >= 0)
        {
            tmp[running].remain--;
            tl[tp++] = tmp[running].pid;
            clock++;
            if (tmp[running].remain == 0)
            {
                tmp[running].finish_time = clock;
                done++;
                running = -1;
            }
        }
        else
        {
            tl[tp++] = 0; // idle
            clock++;
        }
    }
    print_gantt(tl, tp);
    calc_result(tmp, n, "SJF-P");
}

/* 4) Priority (비선점, 작은 priority 값 = 높은 우선순위) */
void prio_np(PCB *P, int n)
{
    PCB tmp[MAX_P];
    memcpy(tmp, P, n * sizeof(PCB)); // 메모리 블록을 바이트 단위로 그대로 복사하는 함수: 원본 배열 P를 임시 배열 tmp로 복사
    Queue rq, ioq;
    q_init(&rq);
    q_init(&ioq);
    // 프로세스 초기 I/O 세팅
    for (int i = 0; i < n; i++)
    {
        tmp[i].remain = tmp[i].burst;
        tmp[i].next_io = 0;
        tmp[i].in_io = 0;
        for (int j = 0; j < tmp[i].io_count; j++)
        {
            tmp[i].io_remaining[j] = tmp[i].io_burst[j];
        }
    }
    int done = 0, clock = 0, tl[MAX_P * 50], tp = 0;
    int running = -1;
    while (done < n)
    {
        check_arrival(tmp, n, clock, &rq);  // 새로 도착한 프로세스 rq push
        tick_io(tmp, n, &ioq);              // I/O 큐의 io_remaining time 1감소
        check_io_finish(tmp, n, &ioq, &rq); // I/O 끝난 경우 ioq pop, rq push

        // I/O 진입 검사
        if (running >= 0)
        {
            int executed = tmp[running].burst - tmp[running].remain;
            if (tmp[running].next_io < tmp[running].io_count && executed == tmp[running].io_req[tmp[running].next_io])
            {
                tmp[running].in_io = 1;
                q_push(&ioq, running);
                running = -1;
            }
        }
        // CPU 쉬는 경우 우선순위 기준으로 새 프로세스 선택
        if (running < 0)
        {
            int sel = -1, minp = 1e9;
            for (int i = 0; i < n; i++)
            {
                if (tmp[i].remain > 0 && tmp[i].in_io == 0 && tmp[i].arrival <= clock)
                {
                    if (tmp[i].priority < minp)
                    {
                        minp = tmp[i].priority;
                        sel = i;
                    }
                }
            }
            running = sel;
        }
        // 1틱 실행
        if (running >= 0)
        {
            tmp[running].remain--;
            tl[tp++] = tmp[running].pid;
            clock++;
            if (tmp[running].remain == 0)
            {
                tmp[running].finish_time = clock;
                done++;
                running = -1;
            }
        }
        else
        {
            tl[tp++] = 0; // idle
            clock++;
        }
    }

    print_gantt(tl, tp);
    calc_result(tmp, n, "Priority-NP");
}

/* 5) Priority (선점) */
void prio_p(PCB *P, int n)
{
    PCB tmp[MAX_P];
    memcpy(tmp, P, n * sizeof(PCB)); // 메모리 블록을 바이트 단위로 그대로 복사하는 함수: 원본 배열 P를 임시 배열 tmp로 복사
    Queue rq, ioq;
    q_init(&rq);
    q_init(&ioq);
    // 프로세스 초기 I/O 세팅
    for (int i = 0; i < n; i++)
    {
        tmp[i].remain = tmp[i].burst;
        tmp[i].next_io = 0;
        tmp[i].in_io = 0;
        for (int j = 0; j < tmp[i].io_count; j++)
        {
            tmp[i].io_remaining[j] = tmp[i].io_burst[j];
        }
    }
    int done = 0, clock = 0, tl[MAX_P * 50], tp = 0;
    int running = -1;
    // 한 틱 단위로 한 사이클 수행하는 반복문
    while (done < n)
    {
        check_arrival(tmp, n, clock, &rq);
        tick_io(tmp, n, &ioq);
        check_io_finish(tmp, n, &ioq, &rq);

        // I/O 진입 검사
        if (running >= 0)
        {
            int executed = tmp[running].burst - tmp[running].remain;
            if (tmp[running].next_io < tmp[running].io_count && executed == tmp[running].io_req[tmp[running].next_io])
            {
                tmp[running].in_io = 1;
                q_push(&ioq, running);
                running = -1;
            }
        }

        // 매 틱마다 선점형 우선순위 기준으로 프로세스 선택
        int sel = -1, minp = 1e9;
        for (int i = 0; i < n; i++)
        {
            if (tmp[i].remain > 0 && tmp[i].in_io == 0 && tmp[i].arrival <= clock) // 레디 큐 존재 조건
            {
                if (tmp[i].priority < minp)
                {
                    minp = tmp[i].priority;
                    sel = i;
                }
            }
        }

        running = sel;

        // 1틱 실행
        if (running >= 0)
        {
            tmp[running].remain--;
            tl[tp++] = tmp[running].pid;
            clock++;
            if (tmp[running].remain == 0)
            {
                tmp[running].finish_time = clock;
                done++;
                running = -1;
            }
        }
        else
        {
            tl[tp++] = 0; // idle
            clock++;
        }
    }

    print_gantt(tl, tp);
    calc_result(tmp, n, "Priority-P");
}

/* 6) Round Robin */
void rr(PCB *P, int n, int q)
{
    PCB tmp[MAX_P];
    memcpy(tmp, P, n * sizeof(PCB)); // 메모리 블록을 바이트 단위로 그대로 복사하는 함수: 원본 배열 P를 임시 배열 tmp로 복사
    Queue rq, ioq;
    q_init(&rq);
    q_init(&ioq);

    // 프로세스 초기 I/O 세팅
    for (int i = 0; i < n; i++)
    {
        tmp[i].remain = tmp[i].burst;
        tmp[i].next_io = 0;
        tmp[i].in_io = 0;
        for (int j = 0; j < tmp[i].io_count; j++)
        {
            tmp[i].io_remaining[j] = tmp[i].io_burst[j];
        }
    }

    int clock = 0, done = 0, tl[MAX_P * 50], tp = 0;
    int running = -1;  // 현재 실행중인 프로세스 인덱스
    int slice_rem = 0; // 남은 타임슬라이스 -> 이거 보장해주고 프로세스 선택

    // 한 틱 단위로 한 사이클 수행하는 반복문
    while (done < n)
    {
        check_arrival(tmp, n, clock, &rq);
        tick_io(tmp, n, &ioq);
        check_io_finish(tmp, n, &ioq, &rq);

        // I/O 진입 검사
        if (running >= 0)
        {
            int executed = tmp[running].burst - tmp[running].remain;
            if (tmp[running].next_io < tmp[running].io_count && executed == tmp[running].io_req[tmp[running].next_io])
            {
                tmp[running].in_io = 1;
                q_push(&ioq, running);
                running = -1;
            }
        }

        // 타임슬라이스 다 됐는지 검사
        if (running >= 0 && slice_rem == 0)
        {
            // burst 남아 있고 I/O 도 아니면, 레디큐 맨 뒤로
            q_push(&rq, running);
            running = -1;
        }

        // 레디 큐에서 새로운 프로세스 꺼내기
        if (running < 0 && !q_empty(&rq))
        {
            running = q_pop(&rq); // 기본적으로 FCFS
            slice_rem = q;        // 새 타임슬라이스 할당
        }
        // 1틱 실행
        if (running >= 0)
        {
            tmp[running].remain--;
            tl[tp++] = tmp[running].pid;
            slice_rem--;
            clock++;
            // 완료 체크
            if (tmp[running].remain == 0)
            {
                tmp[running].finish_time = clock;
                done++;
                running = -1;
            }
        }
        else
        {
            tl[tp++] = 0;
            clock++;
        }
    }

    print_gantt(tl, tp);
    calc_result(tmp, n, "Round Robin");
}

/* --------------------- 입출력화면 ---------------------  */
void menu()
{
    printf("\n------ CPU Scheduling Simulator ------\n");
    printf(" 1. FCFS\n");
    printf(" 2. SJF (Non-preemptive)\n");
    printf(" 3. SJF (Preemptive)\n");
    printf(" 4. Priority (Non-preemptive)\n");
    printf(" 5. Priority (Preemptive)\n");
    printf(" 6. Round Robin\n");
    printf(" 0. Exit\n");
    printf("Choose: ");
    fflush(stdout); // <- 출력이 버퍼에 남지 않도록 강제 출력
}

int main(void)
{
    PCB P[MAX_P];
    int n, mode, sel;
    int time_slice;

    // 1) 프로세스 개수 입력
    printf("Enter number of processes: ");
    scanf("%d", &n);
    if (n < 1 || n > MAX_P)
    {
        printf("Invalid process count (1~%d)\n", MAX_P);
        return 1;
    }
    // 2) RR 전용 time quantum 설정
    printf("Enter time quantum for RR: ");
    scanf("%d", &time_slice);

    // 3) 모드 선택
    printf("Select mode: 1) Random  2) Manual input: ");
    scanf("%d", &mode);

    // 4) 생성 함수 호출
    if (mode == 1)
    {
        // 랜덤 생성
        create_processes(P, n);
    }
    else
    {
        // 수동 입력
        input_processes(P, n);
    }

    // 5) 스케줄링 메뉴
    while (1)
    {
        menu();
        if (scanf("%d", &sel) != 1) // 정수가 아니면 반복문 종료
            break;
        switch (sel)
        {
        case 1:
            fcfs(P, n);
            break;
        case 2:
            sjf_np(P, n);
            break;
        case 3:
            sjf_p(P, n);
            break;
        case 4:
            prio_np(P, n);
            break;
        case 5:
            prio_p(P, n);
            break;
        case 6:
            rr(P, n, time_slice);
            break;
        case 0:
            return 0;
        default:
            puts("Invalid");
            break;
        }
    }
    return 0;
}
