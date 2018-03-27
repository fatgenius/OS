#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


struct sum_runner_struct{
    long long limit;
    long long answer;
};

void* sum_runner(void* arg)
{
  struct sum_runner_struct *arg_struct=
          (struct sum_runner_struct*) arg;
//long long *limit_ptr = (long long*) arg;
  //long long limit =*limit_ptr;
  long long sum =0;
  for (long long i =0; i < arg_struct->limit; i++) {
    sum +=i;
  }
  arg_struct-> answer= sum;
  pthread_exit(0);
}

 int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <num1><num2>...<num-n>\n",argv[0]);
    exit(-1);
  }
  int num_args = argc -1;
  struct sum_runner_struct args[num_args];//hread id
  pthread_t tid[num_args];
  for (int i = 0; i < num_args; i++)
  {
    //struct sum_runner_struct args;
    args[i].limit = atoll(argv[i+1]);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&tid[i], &attr, sum_runner,& args[i]);
    /* code */
  }


//  pthread_create(&tid, &attr, sum_runner,& limit);
//wait until thread is done
for (int i = 0; i < num_args; i++) {

    pthread_join(tid[i], NULL);/* code */
    printf("sum is %lld\n", args[i].answer );
}



}
