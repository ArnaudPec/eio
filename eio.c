// gcc -o eio_file_ls eio.c `pkg-config --cflags --libs ecore eio eina` -g
/* CPU usage cal
 * https://stackoverflow.com/questions/16726779
 /how-do-i-get-the-total-cpu-usage-of-an-application-from-proc-pid-stat*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <Eio.h>
#include <Ecore.h>

static Eina_List *processesses_list, *l;

typedef struct process process_t;
struct process
{
   int pid;
   const char * name;
   int mem;
   double cpu;
};

static Eina_Bool
get_process_information(const char * process, process_t * proc);

void
print_process(process_t * p)
{
   printf("%d %s %d %f\n", p->pid, p->name, p->mem, p->cpu);
}

static Eina_Bool
_filter_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const char *file)
{
   char *last_slash = strrchr(file, '/');
   return (last_slash[1] > '9') ? EINA_FALSE : EINA_TRUE;
}

int
sort_cb(const void *d1, const void *d2)
{
   const process_t *p1 = d1;
   const process_t *p2 = d2;
   if(!p1) return(1);
   if(!p2) return(-1);

   return (p1->mem <= p2->mem) ? 1 : -1;
}

static void
_main_cb(void *data, Eio_File *handler EINA_UNUSED, const char *file)
{
   process_t *p;
   p = calloc(1, sizeof(process_t));

   get_process_information(file, p);
   processesses_list = eina_list_sorted_insert(processesses_list, sort_cb, p);
}

static void
_done_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   process_t *w;

   EINA_LIST_FOREACH(processesses_list, l, w)
      print_process(w);

   printf("processesses_list count: %d\n", eina_list_count(processesses_list));

   ecore_main_loop_quit();
}

static void
_error_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, int error)
{
   fprintf(stderr, "Something has gone wrong:%s\n", strerror(error));
   ecore_main_loop_quit();
}

static Eina_Bool
get_process_information(const char * file, process_t * proc)
{
   FILE *f = NULL;
   char stat_path[100] = {0}, statm_path[100] = {0};
   char buf[400], bufm[400], bufu[400];
   unsigned long ttime, utime, stime, cstime, cutime, starttime, hertz;
   double secs, uptime;
   int i = 0;

   strcat(stat_path, file);
   strcat(stat_path, "/stat");

   f = fopen(stat_path, "r+");
   if (!f) return EINA_FALSE;

   while (fgets(buf, sizeof(buf), f) != NULL)
      buf[strlen(buf) - 1] = '\0';

   fclose(f);

   for (char *p = strtok(buf," "); p != NULL; p = strtok(NULL, " "))
   {
      switch (i++) {
         case 0:
            proc->pid = atoi(p);
            break;
         case 1:
            proc->name = eina_stringshare_add(p);
            break;
         case 13:
            utime = atol(p);
            break;
         case 14:
            stime = atol(p);
            break;
         case 15:
            cutime = atol(p);
            break;
         case 16:
            cstime = atol(p);
            break;
         case 21:
            starttime = atol(p);
            break;
      }
   }

   strcat(statm_path, file);
   strcat(statm_path, "/statm");

   f = fopen(statm_path, "r+");
   if (!f) return EINA_FALSE;

   while (fgets(bufm, sizeof(bufm), f) != NULL)
      bufm[strlen(bufm) - 1] = '\0';

   fclose(f);

   i = 0;
   for (char *p = strtok(bufm," "); p != NULL; p = strtok(NULL, " "))
   {
      switch (i++) {
         case 0:
            proc->mem = atoi(p);
            break;
      }
      break;
   }

   hertz = sysconf(_SC_CLK_TCK);

   f = fopen("/proc/uptime", "r+");
   if (!f) return EINA_FALSE;

   while (fgets(bufu, sizeof(bufu), f) != NULL)
      bufu[strlen(bufu) - 1] = '\0';

   fclose(f);

   i = 0;
   for (char *p = strtok(bufu," "); p != NULL; p = strtok(NULL, " "))
   {
      switch (i++) {
         case 0:
            {
               uptime = atof(p);
               break;
            }
      }
   }

   ttime = utime + stime + cstime + cutime;
   secs = uptime - (double) (starttime / hertz);
   proc->cpu = 100 * ((double) (ttime / hertz) / secs);

   return EINA_TRUE;
}

int
main(int argc, char **argv)
{
   process_t *w;
   processesses_list = NULL;

   eina_init();
   ecore_init();
   eio_init();

   if (argc < 2)
   {
      fprintf(stderr, "Missing path\n");
      return -1;
   }

   eio_file_ls(argv[1], _filter_cb, _main_cb, _done_cb, _error_cb, NULL);

   ecore_main_loop_begin();
   EINA_LIST_FREE(processesses_list, w)
   {
      eina_stringshare_del(w->name);
      free(w);
   }

   eio_shutdown();
   ecore_shutdown();
   eina_shutdown();

   return 0;
}
