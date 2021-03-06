#include "libelfsh.h"

#define BUF_SIZE 1024
#define ARR_SIZE 4098

int get_sect_index(char *);

int main(int argc, char **argv) {
  elfshobj_t *in_file;
  FILE *syms_file;
  char *out_file;
  int ret;
  elfsh_Sym sym;
  char **name = (char**) malloc(ARR_SIZE * sizeof(char*));
  u_int idx;

  int tot_syms = 0;
  long int *addr = (long int*) malloc(ARR_SIZE * sizeof(long int));
  long int *size = (long int*) malloc(ARR_SIZE * sizeof(long int));
  int *section = (int *) malloc(ARR_SIZE * sizeof(int));

  if(argc < 4) {
    printf("Usage: inject <input_file> <symbols_file> <output_file>\n");
    exit(1);
  }

  /* get input file from command line arguments */
  in_file = elfsh_map_obj(argv[1]);
  if(!in_file) {
    printf("Error loading input file: aborting...\n");
    elfsh_error();
    exit(1);
  }

  /* get the symbols file from command arguments */
  syms_file = fopen(argv[2], "r");
  if(!syms_file) {
    printf("Error loading symbols file: aborting...\n");
    elfsh_error();
    exit(1);
  }
  
  /* set up output file */
  out_file = argv[3];

  /* read symbols file and fill addr, size and type arrays */
  char *line = NULL;
  char temp[BUF_SIZE];
  int temp_indx;
  char *tok;
  size_t read;
  /* loop assumes correct input <name, 0x0000addr, size> */
  while((read = getline(&line, &read, syms_file)) != -1) {
    //    printf("%d\n", read);
    //printf("%s\n", line);
    strcpy(temp, line);
    temp_indx = 0;
    while(temp[temp_indx + 1]) {
      temp[temp_indx] = temp[temp_indx + 1];
      temp_indx++;
    }
    temp[strlen(temp) - 3] = '\0';
    //printf("%s\n", temp);
    
    /* get symbol name */
    tok = strtok(temp, ",");
    //printf("%s\n", tok);
    name[tot_syms] = (char*) malloc(strlen(tok) + 1);
    strcpy(name[tot_syms],tok);
  
    /* get symbol address */
    tok = strtok(NULL, ",");
    addr[tot_syms] = strtol(tok, NULL, 0);
    //printf("%li\n", addr[tot_syms]);

    /* get symbol size */
    tok = strtok(NULL, ",");
    size[tot_syms] = strtol(tok, NULL, 0);
    //printf("%li\n", size[tot_syms - 1]);
   
    tok = strtok(NULL, ",");
    section[tot_syms++] = get_sect_index(tok);
  }

  //printf("---------------------------------------------------------");
  int i;
  /*for(i = 0; i < tot_syms; i++) {
    printf("%s, 0x%li, %li\n", name[i], addr[i], size[i]);
    }*/

  printf("injecting symbols...\n");
  /* get the symbol information */
  for(i = 0; i < tot_syms; i++) {    
    sym = elfsh_create_symbol(addr[i], size[i], STT_FUNC, 1, 0, section[i]);
    ret = elfsh_insert_symbol(in_file->secthash[ELFSH_SECTION_SYMTAB], &sym, name[i]);
    if(ret < 0) {
      elfsh_error();
      exit(-1);
    }
  }

  ret  = elfsh_save_obj(in_file, out_file);
  if(ret < 0) {
    elfsh_error();
  } 

  printf("Relinking *%s* \n", ret ? "Error" : "OK");
  return 0;
}

int get_sect_index(char * sect_name) {
    if(!(strcmp(sect_name, " .text"))) {
        return 6;
    }
    else if(!strcmp(sect_name, " __libc_freeres_fn")) {
        return 7;
    }
    else if(!strcmp(sect_name, " .fini")) {
        return 9;
    }
    else
        return 0;
}
