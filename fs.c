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



int verifica_formatacao(){
  int i=0;
  while(i<32 && fat[i] == 3)
  {
    // printf("fat[i]: %d\n", fat[i]);
    i++;
  }

  //se estiver formatado errado
  // printf("i: %d\n", i);
  if (i!=32) //disco novo
  {
    printf("sistema de arquivos não formatado\n");
    return 0;
  }

  //se tiver formatado correto
  return 1;
}

int fs_init() {  
  //verificar se esta iniciado ou é disco novo
  //se os 32 vprimeios espaços da fat são 3, então já foi inicado
 
  int sector=0;
  for(;sector<32;sector++)
    bl_read(sector, (char *) fat + SECTORSIZE * sector);

  for(;sector<33;sector++){
    bl_read(sector, (char *) fat + SECTORSIZE * sector);
    bl_read(sector, (char *) dir);
  }

  //verifica se está formatado
  int i=0;
  while(i<32 && fat[i] == 3)
  {
    i++;
  }
 
  if (i!=32) //disco novo
  {
    //se disco não estiver formatado, formata ele
    fs_format();
  }

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
  	memset(dir[i].name, ' ', 25*sizeof(char)); //inicializa o nome da string com " " em todas as celulas.
  	dir[i].first_block = 0;
  	dir[i].size = 0;
  }

  //escrever as duas estruturas no disco
  for (int sector = 0; sector < 32; sector++)
  {
    bl_write(sector, (char *) fat + SECTORSIZE * sector);
  }
  
  bl_write(32, (char *) dir);
  return 1;
}

//retorna o espaco livre no dispositivo (disco) em bytes.
int fs_free() {
  //verifica se esta formatado
  if(!verifica_formatacao()){
    return 0;
  }

  int celula_livre = 0;
  //fat representa o disco, entao o espaco que tem livre nele e o que livre no disco, exceto os quebrados
  for(int i=0;i<FATCLUSTERS;i++){
    // printf("%d ", fat[i]);
    if(fat[i] == 1){
      celula_livre++;
    }
  }
  //cada celula equivale a um setor, cada setor tem 4096 bytes (isto eh, um clustersize)
  int total_bytes = celula_livre * CLUSTERSIZE;
  return total_bytes;
}

// int fs_list(char *buffer, int size): Lista os arquivos do diretório, colocando a saída formatada em buffer. O formato é simples, um arquivo
// por linha, seguido de seu tamanho e separado por dois tabs. Observe
// que a sua função não deve escrever na tela.
int fs_list(char *buffer, int size) {
  if(!verifica_formatacao()){
    return 0;
  }

  //reseta o vetor buffer e apaga tudo que já estava escrito nele
  memset(buffer, '\0', size);

  int tamanho_usado = 0;
  for(int i=0;i<DIRENTRIES;i++){
    if(dir[i].used == 1){
      char temp[50];
      snprintf(temp, sizeof(temp), "%-25s %d    \n", dir[i].name, dir[i].size);
      if(tamanho_usado + strlen(temp) < size){
        strcat(buffer, temp);
      }
      else{
        printf("Erro. Buffer cheio!\n");
        return 0;
      } 
    }
  }
  return 1;
}

int fs_create(char* file_name) {
  if(!verifica_formatacao()){
    return 0;
  }

  //verifica se ja tem arquivo com este nome
  for(int i=0;i<DIRENTRIES;i++){
    if(!strcmp(file_name, dir[i].name)){ //se arquivo dir tem msm nome 
      printf("Erro! Ja existe um arquivo com esse nome\n");;
      return 0;
    }
  }

  //procura uma celuala livre no FAT
  int primeiro_bloco = -1;
  for(int i=0;i<FATCLUSTERS;i++){
    if(fat[i] == 1){
      fat[i] = 2; //marca essa celula como incio e fim de um arquivo (o arquivo tem tamanho 0, por isso, o inicio e o fim eh igual)
      primeiro_bloco = i;
      break;
    }
  }
  
  if(primeiro_bloco == -1){
    printf("FAT sem espaco\n");
    return 0;
  }

  for(int i=0;i<DIRENTRIES;i++){
    if(!dir[i].used){ //se celula esta livre
      dir[i].used=!dir[i].used;
      strcpy(dir[i].name,file_name);
      dir[i].size = 0;
      dir[i].first_block = primeiro_bloco;
      break;
    }
  }

  return 1;
}

int fs_remove(char *file_name) {
  if(!verifica_formatacao()){
    return 0;
  }
  
  //procura a celuala no FAT
  for(int i=0;i<128;i++){
    if(!strcmp(file_name, dir[i].name)){ //se arquivo dir tem msm nome do arquivo para remover
      dir[i].used = 0;
      memset(dir[i].name, ' ', 25*sizeof(char)); //inicializa o nome da string com " " em todas as celulas.
      dir[i].size = 0;

      int bloco_procurar = dir[i].first_block;
      int temp = -1;
      while(temp != 2){
        temp = fat[bloco_procurar];
        fat[bloco_procurar] = 1;
        bloco_procurar = fat[temp]; //quando temp = 2, nao acontece nada de ruim
      }
      dir[i].first_block = 1;
      return 1;
    }
  }
  printf("Não existe arquivos com esse nome\n");
  return 0;
}

// -------- PARTE 2 ------------------
int fs_open(char *file_name, int mode) {
  if (mode == FS_R){ //se for abeerto em modo leitura, deve retornar -1 se arquivo nao existir
    int achou = 0;
    for(int i=0;i<128;i++){
      if(!strcmp(dir[i].name, file_name)){
        achou = 1;
        break;
      }
    }
    if(!achou) return -1;
  }

//  Ao abrir um arquivo para escrita, o arquivo deve ser criado ou um arquivo pré-existente 
// deve ser apagado e criado novamente com tamanho
// 0. Retorna o identificador do arquivo aberto, um inteiro, ou -1 em caso
// de erro.
  else if(mode == FS_W){
    int result = fs_create(file_name);
    if(result == -1){
      fs_remove(file_name);
      fs_create(file_name);
    }
  }
  return 0;
}

int fs_close(int file)  { //file indico o primeiro bloco no FAT?
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