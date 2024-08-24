/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010,2012,2019 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096
#define FATCLUSTERS 65536
#define DIRENTRIES 128

unsigned short fat[FATCLUSTERS];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];


int fs_init() {  
  printf("Função não implementada: fs_init\n");
  return 1;
}

int fs_format() {
    //inicializando FAT
  int i=0;
  for(;i<32;i++){
  	fat[i] = 3;
  }
  
  for(;i<33;i++){
  	fat[i] = 4;
  }

  for(;i<FATCLUSTERS;i++){
  	fat[i] = 1;
  }
  //inicializando Diretório
  i=0;
  for(;i<128;i++){
  	dir[i].used = 0;
  	memset(dir[i].name, " ", 25*sizeof(char)); //inicializa o nome da string com " " em todas as celulas.
  	dir[i].first_block = 0;
  	dir[i].size = 0;
  }

  //escrever as duas estruturas no disco
  // bl_write(qual_setor,o_que_escrever);
  return 0;
}

//retorna o espaco livre no dispositivo (disco) em bytes.
int fs_free() {
  //duvida: contar as celulas que estao parcialmente ocupadas por um arquivo?
  int celula_livre = 0;
  //fat representa o disco, entao o espaco que tem livre nele e o que livre no disco, exceto os quebrados
  for(int i=0;i<FATCLUSTERS;i++){
    if(fat[i] == 1){
      celula_livre++;
    }
  }
  //cada celula equivale a um setor, cada setor tem 4096 bytes (isto eh, um clustersize)
  int total_bytes = celula_livre * CLUSTERSIZE;
  //nao tem printf no nucleo de um SO, portanto, devemos passar os bytes livres de alguma outra forma.
  printf("total bytes livres: %d\n", total_bytes);
  return 0;
}

// int fs_list(char *buffer, int size): Lista os arquivos do diretório, colocando a saída formatada em buffer. O formato é simples, um arquivo
// por linha, seguido de seu tamanho e separado por dois tabs. Observe
// que a sua função não deve escrever na tela.
int fs_list(char *buffer, int size) {
  //duvida: esse buffer tem 4096 bytes ou eh um vetor de bytes qualquer?
  int aux = 1;
  int tamanho_usado = 0;
  for(int i=0;i<DIRENTRIES;i++){
    if(dir[i].used){
      //nao pode usar printf, tem que passar para um arquivo ou coisa assim
      //esse eh o formato certo?
      char temp[50];
      snprintf(temp, sizeof(temp), "%-25s %d    ", dir[i].name, dir[i].size);
      if(tamanho_usado + len(temp) < size){
        strcat(buffer, temp);
      }
      else{
        printf("Erro. Buffer cheio!\n");
        return -1;
      } 
      // printf("%s%d   \n",dir[i].name, dir[i].size); //esse eh o formato certo?
    }
  }
  return 0;
}

int fs_create(char* file_name) {
  //procura uma celuala livre no FAT
  for(int i=0;i<FATCLUSTERS;i++){
    if(i<=128){
      if(!strcmp(file_name, dir[i].name)){ //se arquivo dir tem msm nome 
        printf("Erro! Ja existe um arquivo com esse nome\n");;
        return -1; 
      }
      if(!dir[i].used){ //se celula esta livre
        dir[i].used=!dir[i].used;
        strcpy(dir[i].name,file_name);
        dir[i].size = 0;
        dir[i].first_block = i;
      }
    }
//
    if(fat[i] == 1){
      fat[i] = 2; //marca essa celula como incio e fim de um arquivo (o arquivo tem tamanho 0, por isso, o inicio e o fim eh igual)
    }
  }
  return 0;
}

int fs_remove(char *file_name) {
  //procura a celuala no FAT
  for(int i=0;i<FATCLUSTERS;i++){
    if(i<=128){
      if(!strcmp(file_name, dir[i].name)){ //se arquivo dir tem msm nome do arquivo para remover
        dir[i].used=!dir[i].used;
        strncpy(dir[i].name," ", 25);
        dir[i].size = 0;
        dir[i].first_block = 0;
      }
    }

    if(fat[i] == 1){
      fat[i] = 2; //marca essa celula como incio e fim de um arquivo (o arquivo tem tamanho 0, por isso, o inicio e o fim eh igual)
    }
  }
  return 0;
}

int fs_open(char *file_name, int mode) {
  printf("Função não implementada: fs_open\n");
  return -1;
}

int fs_close(int file)  {
  printf("Função não implementada: fs_close\n");
  return 0;
}

int fs_write(char *buffer, int size, int file) {
  printf("Função não implementada: fs_write\n");
  return -1;
}

int fs_read(char *buffer, int size, int file) {
  printf("Função não implementada: fs_read\n");
  return -1;
}