#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void *blink(void *arg) {
  for(int i=0; i<10; i++) {
    printf("\033[31mHELLO\033[0m"); // 紅色字串
    fflush(stdout); // 刷新緩衝區
    usleep(500000); // 暫停 0.5 秒
    printf("\033[0K"); // 清除本行內容
    fflush(stdout); // 刷新緩衝區
    usleep(500000); // 暫停 0.5 秒
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t tid;
  pthread_create(&tid, NULL, blink, NULL);
  pthread_join(tid, NULL);
  printf("\n");
  return 0;
}
