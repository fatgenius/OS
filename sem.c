#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 5 //number of software developers
#define K 8 //number of end users

sem_t sd[N];                  // used to pause or resume software developer work
sem_t critical;               // used to check Egbert state and manage the waiting room (only one sd can enter room at time)
sem_t critical_2;             // be sure that users don't send new reports, while reading number of reports and sending invites
sem_t critical_3;             // used to manage "sem_t waiting" between software and user meeting threads
sem_t critical_4;             // used to safely check if user_meeting is running (or if there are new reports from user)
sem_t egbert;                 // when zero - Egbert is on meeting, when one - Egbert is free
sem_t waiting;                // used to track the number of waiting software developers
sem_t user_report;            // the number of user reports send
sem_t invite;                 // user wait for invite
sem_t meeting_invite;         // the number of send invites
sem_t um_running;             // when zero - user meeting is running
sem_t user_meeting_finished;  // when one - user meeting finished
sem_t sm_runnung;             // when zero - software meeting is running
sem_t user_arrived;           // indicates if user has arrived to the company

int leader_state = 0; // variable to store current Egbert state
int userKing = 1;     // variable to store state of user meeting
int waiting_room[5];  // when software changes its state to waiting it will be added to this array
int j = 0;            // waiting room place counter
int p = 0;            // number of users in waiting list

// thread that simulates the work of software developer
void *tsoftdev(void *ptr)
{
    int i, k = *((int *) ptr);
    for(i = 0; i <= 10; i++) {
        printf("Software developer %d is working for %d\n", k + 1, i);
        sleep(1);
        sem_wait(&critical);
        sem_getvalue(&egbert, &leader_state);
        // if Egbert is not on meeting
        if(leader_state == 1)
        {
            // software developer goes to waiting room
            waiting_room[j] = k + 1;
            j++;
            printf("Software developer %d is waiting\n", k + 1);
            sem_post(&waiting);
            sem_post(&critical);
            sem_wait(&sd[k]);
            // software developer left waiting room
        }
        else
        {
            sem_post(&critical);
        }
    }
    pthread_exit(0);
}

// thread that simulates the user
void *tuser(void *ptr)
{
    int k = *((int *) ptr);
    int time;

    sem_wait(&critical_2);
    sem_post(&user_report);
    printf("user %d send report\n", k + 1);
    sem_post(&critical_2);

    printf("user %d wait for invite\n", k + 1);
    sem_wait(&invite);
    printf("user %d invited\n", k + 1);

    printf("user %d travel to company\n", k + 1);
    sleep(5);
    sem_post(&user_arrived);
    printf("user %d arrived at the company\n", k + 1);
    printf("user %d is waiting for meeting\n", k + 1);
    sem_wait(&meeting_invite);
    for(time = 0; time < 10; time++)
    {
        printf("User %d is on meeting for %d\n", k + 1, time);
        sleep(1);
    }
    sem_wait(&user_meeting_finished);
    printf("User %d left company\n", k + 1);
    pthread_exit(0);
}

// this function is used to check if any report arrived during the preparation for software meeting
void check_user_meeting_running(int n_of_waiting_sd)
{
    int i = 0;
    sem_wait(&critical_4);
        sem_getvalue(&um_running, &userKing);
        // if report received from the user
        if(userKing == 0)
        {
            sem_post(&critical_4);
            for(i = 0; i < n_of_waiting_sd; i++) {
                sem_post(&waiting);
            }
            sem_post(&critical_3);
            printf("cancel software meeting\n");
            pthread_exit(0);
        }
}

// thread that simulates the software meeting
void *software_meeting(void *ptr)
{
        int i;
        int time;
        sem_wait(&waiting);
        check_user_meeting_running(1);
        sem_post(&critical_4);
        sem_wait(&waiting);
        check_user_meeting_running(2);
        sem_post(&critical_4);
        sleep(5); // report might arrive
        sem_wait(&waiting);
        check_user_meeting_running(3);

        sem_wait(&critical);
        sem_wait(&egbert);
        sem_post(&critical);
        sem_wait(&sm_runnung);
        sem_post(&critical_4);
        sem_post(&critical_3);

        // if more than three software developers are in waiting room
        // let them continue to work
        for(i = 3; i < 5; i++)
        {
            if(waiting_room[i] != 0)
            {
                sem_post( &sd[waiting_room[i] - 1] );
                waiting_room[i] = 0;
                sem_wait(&waiting);
            }
        }

        for(time = 0; time < 10; time++)
        {
            for(i = 0; i < 3; i++)
            {
               printf("Software developer %d is on meeting for %d\n", waiting_room[i], time);
            }
            printf("Egbert is on meeting for %d\n", time);
            sleep(1);
        }

        printf("software meeting finished\n");
        for(i = 0; i < 3; i++)
        {
            sem_post( &sd[waiting_room[i] - 1] );
            waiting_room[i] = 0;
        }
        j = 0; //!!!
        sem_post(&sm_runnung);
        sleep(5);
        sem_wait(&critical);
        sem_post(&egbert);
        printf("Egbert is free\n");
        sem_post(&critical);
        pthread_exit(0);

}

// thread that simulates the user meeting
void *user_meeting(void *ptr)
{
    int i;
    int time;
    int n_of_reports;
    while(1)
    {
        sem_wait(&user_report); // wait for report from user
        printf("report arrived\n");
        sem_wait(&sm_runnung);
        sem_post(&sm_runnung);
        sem_wait(&waiting);  // wait for one software developer
        sem_wait(&critical);
        sem_wait(&egbert);
        sem_post(&critical);
        sleep(3);

        sem_wait(&critical_2);
        sem_post(&invite);
        printf("send invite to user\n");
        sem_getvalue(&user_report, &n_of_reports);
        printf("number of reports %d\n", n_of_reports);
        for(i = 0; i < n_of_reports; i++)
        {
            printf("wait for reports\n");
            sem_wait(&user_report);
            printf("reports received\n");
            sem_post(&invite);
            printf("send invite to user\n");

        }
        sem_post(&critical_2);

        for(i = 0; i < n_of_reports + 1; i++)
        {
            sem_wait(&user_arrived);
        }

        sem_wait(&critical_3);
        sem_post(&critical_3);
        for(i = 1; i < 5; i++)
        {
            if(waiting_room[i] != 0)
            {
                sem_post( &sd[waiting_room[i] - 1] );
                printf("sd %d is free\n", waiting_room[i]);
                waiting_room[i] = 0;
                sem_wait(&waiting);
            }
        }

        for(i = 0; i < n_of_reports + 1; i++)
        {
            sem_post(&meeting_invite);
        }

        for(time = 0; time < 10; time++)
        {
            printf("Egbert is on meeting for %d\n", time);
            printf("Software developer %d is on meeting for %d\n", waiting_room[0], time);
            sleep(1);
        }

        for(i = 0; i < n_of_reports + 1; i++)
        {
            sem_post(&user_meeting_finished);
        }

        printf("user meeting finished\n");
        sem_post( &sd[waiting_room[0] - 1] );
        waiting_room[0] = 0;
        j = 0; //!!

        sleep(5);
        sem_wait(&critical);
        sem_post(&egbert);
        printf("Egbert is free\n");
        sem_post(&critical);
    }
}

// this function is used for software meeting initialization
void start_software_meeting()
{
    pthread_t sm_thread;
    sem_wait(&sm_runnung); // to prevent starting new software meeting
    sem_post(&sm_runnung); // before first is finished
    sem_wait(&um_running);
    sem_post(&um_running);
    sem_wait(&critical_3);
    pthread_create(&sm_thread, NULL, &software_meeting, (void *)0);
}

// this function is used for user meeting initialization
void start_user_meeting(int n_of_reports)
{
    int i;
    pthread_t user_thread[n_of_reports];
    int user_targ[n_of_reports];

    sem_wait(&critical_4);
    sem_wait(&um_running);
    sem_post(&critical_4);
    for(i = 0;i < n_of_reports; i++) {
        user_targ[i] = i;
        pthread_create(&user_thread[i], NULL, &tuser, (void *)&user_targ[i]);
        sleep(1);
    }
    for(i = 0; i < n_of_reports; i++) {
        pthread_join(user_thread[i], NULL);
    }
    sem_wait(&critical_4);
    sem_post(&um_running);
    sem_post(&critical_4);
}

int main()
{
    int i;
    int sd_targ[N];
    pthread_t sd_thread[N];
    pthread_t user_thread[K];
    pthread_t um_thread;

    for(i = 0; i < N; i++) {
        sem_init(&sd[i], 0, 0);
    }

    sem_init(&waiting, 0, 0);
    sem_init(&egbert, 0, 1);
    sem_init(&critical, 0, 1);
    sem_init(&critical_2, 0, 1);
    sem_init(&critical_3, 0, 1);
    sem_init(&critical_4, 0, 1);
    sem_init(&user_report, 0, 0);
    sem_init(&invite, 0, 0);
    sem_init(&um_running, 0, 1);
    sem_init(&user_arrived, 0, 0);
    sem_init(&meeting_invite, 0, 0);
    sem_init(&user_meeting_finished, 0, 0);
    sem_init(&sm_runnung, 0, 1);

    for(i = 0;i < N; i++) {
        sd_targ[i] = i;
        pthread_create(&sd_thread[i], NULL, &tsoftdev, (void *)&sd_targ[i]);
    }

    pthread_create(&um_thread, NULL, &user_meeting, (void *)0);

    // test scenario
    start_software_meeting();
    sleep(30);
    start_user_meeting(8);
    start_software_meeting();
    start_user_meeting(2);
    // end of test scenario

    for(i = 0; i < N; i++) {
        pthread_join(sd_thread[i], NULL);
    }

    for(i = 0;i < N; i++) {
        sem_destroy(&sd[i]);
    }

    sem_destroy(&waiting);
    sem_destroy(&egbert);
    sem_destroy(&critical);
    sem_destroy(&critical_2);
    sem_destroy(&critical_3);
    sem_destroy(&critical_4);
    sem_destroy(&user_report);
    sem_destroy(&invite);
    sem_destroy(&um_running);

    return 0;
}
