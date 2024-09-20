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
#define DIRINDEX 32
#define MAXOPENFILES 10

unsigned short fat[FATCLUSTERS];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];


typedef struct {
  int primeiro;
  int fim;
  int dirIndex;
  int posicaoEscrita;
  int posicaoLeitura;
  int totalLido;
  char categoria;
  char ocupado;
  char memoria[CLUSTERSIZE];
} arquivosAbertos;

arquivosAbertos listaArquivos[DIRENTRIES];

/* funções para escrita das estruturas de dados no disco */
void escreve_disco(){
	char *fatBytes;
	char buffer[CLUSTERSIZE];

	// percorre a fat por bytes e anota as informações dentro do buffer depois escreve no disco
	fatBytes = (char *)fat;
	for (int i=0,h=0;i<FATCLUSTERS>>1;i+=CLUSTERSIZE,h++){
		for (int j=i,k=0;j<CLUSTERSIZE;j++,k++)
			 buffer[k] = fatBytes[j];

		bl_write(h, buffer);
	}
}

void escreve_dir_disco(){
	char buffer[CLUSTERSIZE];
	int tam_bytes_dir = sizeof(dir_entry) * DIRENTRIES;
	char *dir_bytes = (char *)dir;

	//percorre o diretório em bytes e escreve neles até o tamanho do diretório
	for(int i=0;i<tam_bytes_dir;i++) buffer[i] = dir_bytes[i];

	//percorre o resto e escreve o resto com 0 se tiver resto
	for(int i=tam_bytes_dir;i<CLUSTERSIZE;i++) buffer[i] = 0;

	bl_write(DIRINDEX, buffer);
}


int verifica_formatacao(){
  int i=0;
  while(i<32 && fat[i] == 3){
    i++;
  }

  //se estiver formatado errado
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
	  //tem que procurar no resto dos arquivos
      break;
    }
  }

  /*escrever o bl_write. Corrigido em relação a primeria entrega*/ 

  escreve_disco();
  escreve_dir_disco();

  return 1;
}

int fs_remove(char *file_name) {
  if(!verifica_formatacao()){
    return 0;
  }
  
  //procura a celuala no FAT
  for(int i=0;i<DIRENTRIES;i++){
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
	
	/*escrever o bl_write. Corrigido em relação a primeria entrega*/ 
	escreve_disco();
  	escreve_dir_disco();
    
	  return 1;
    }
  }

  printf("Erro! Arquivo não encontrado!\n");

  return 0;
}

// -------- PARTE 2 ------------------
int fs_open(char *file_name, int mode) {
  // Busca o arquivo no diretório
  int arquivo_encontrado = -1;  // Variável para armazenar o índice do arquivo no diretório

  // Percorre todas as entradas do diretório para procurar o arquivo pelo nome
  for (int i = 0; i < DIRENTRIES; i++) {
    if (!strcmp(dir[i].name, file_name)) {  // Se encontrar o arquivo, armazena seu índice
      arquivo_encontrado = i;
      break;
    }
  }

  // Verifica se o arquivo foi encontrado no diretório
  if (arquivo_encontrado == -1) {
    printf("Erro! O arquivo não foi encontrado!\n");  // Mostra mensagem de erro se o arquivo não foi encontrado
    return -1;  // Retorna -1 indicando falha
  }

  // Se o modo de abertura for para escrita (FS_W)
  if (mode == FS_W) {
    // Remove o arquivo existente e recria um novo com o mesmo nome
    fs_remove(file_name);  // Remove o arquivo antigo
    fs_create(file_name);  // Cria um novo arquivo com tamanho zero

    // Procura novamente o arquivo recém-criado no diretório
    for (int i = 0; i < DIRENTRIES; i++) {
      if (!strcmp(dir[i].name, file_name)) {
        arquivo_encontrado = i;  // Atualiza o índice para o arquivo recriado
        break;
      }
    }
  } else {
    // Caso o modo seja de leitura, carrega o primeiro bloco do arquivo para a memória
    bl_read(dir[arquivo_encontrado].first_block, listaArquivos[arquivo_encontrado].memoria);
  }

  // Configura as informações iniciais para o arquivo aberto
  arquivosAbertos *arquivo = &listaArquivos[arquivo_encontrado];
  arquivo->primeiro = dir[arquivo_encontrado].first_block;  // Armazena o primeiro bloco do arquivo
  arquivo->fim = arquivo->primeiro;  // Inicializa o bloco final como o primeiro bloco
  arquivo->categoria = mode;  // Define o modo de abertura (leitura ou escrita)
  arquivo->ocupado = 1;  // Marca o arquivo como "ocupado" (aberto)
  arquivo->dirIndex = arquivo_encontrado;  // Armazena o índice do arquivo no diretório
  arquivo->posicaoEscrita = 0;  // Inicializa a posição de escrita no arquivo
  arquivo->posicaoLeitura = 0;  // Inicializa a posição de leitura no arquivo
  arquivo->totalLido = 0;  // Inicializa o total de bytes lidos como zero

  return arquivo_encontrado;  // Retorna o índice do arquivo no diretório
}


int fs_close(int file) {
  // Obtém a estrutura do arquivo correspondente ao descritor fornecido
  arquivosAbertos *arquivo = &listaArquivos[file];

  // Verifica se o arquivo foi aberto para escrita (FS_W)
  if (arquivo->categoria == FS_W) {
    // Grava qualquer conteúdo restante no buffer para o bloco final do arquivo
    bl_write(arquivo->fim, arquivo->memoria);

    // Atualiza o tamanho do arquivo no diretório com a quantidade de bytes escritos
    dir[arquivo->dirIndex].size += arquivo->posicaoEscrita;

    // Salva o diretório atualizado no disco
    escreve_dir_disco();
  }

  // Verifica se o arquivo realmente está aberto (ocupado)
  if (!arquivo->ocupado) {
    // Se o arquivo não estiver aberto, exibe uma mensagem de erro e retorna -1
    printf("Erro: arquivo com file %d não está aberto.\n", file);
    return -1;
  }

  // Marca o arquivo como fechado (não ocupado)
  arquivo->ocupado = 0;

  // Retorna 0 indicando que o fechamento foi bem-sucedido
  return 0;
}


int fs_write(char *buffer, int size, int file) {
  // Obtém a estrutura do arquivo que está sendo escrito
  arquivosAbertos *arquivo = &listaArquivos[file];

  // Verifica se o arquivo está aberto para escrita e se está em uso
  if (arquivo->categoria != FS_W || !arquivo->ocupado) {
    return -1;  // Retorna -1 se o arquivo não estiver no modo de escrita ou não estiver aberto
  }

  // Loop para escrever os dados do buffer no arquivo
  for (int i = 0; i < size; i++) {
    // Escreve um byte do buffer no local atual de escrita do arquivo
    arquivo->memoria[arquivo->posicaoEscrita++] = buffer[i];

    // Verifica se atingiu o limite do bloco (tamanho do cluster)
    if (arquivo->posicaoEscrita == CLUSTERSIZE) {
      // Grava o conteúdo atual do buffer no disco no bloco final do arquivo
      bl_write(arquivo->fim, arquivo->memoria);

      // Procura por um novo bloco livre na FAT (File Allocation Table)
      int novoBloco = -1;
      for (int j = DIRINDEX + 1; j < FATCLUSTERS; j++) {
        if (fat[j] == 1) {  // Se encontrar um bloco livre (valor 1 na FAT)
          novoBloco = j;    // Atribui esse bloco como o novo bloco
          break;            // Encerra a busca
        }
      }

      // Atualiza a FAT com o novo bloco alocado
      fat[arquivo->fim] = novoBloco;
      arquivo->fim = novoBloco;  // Atualiza o bloco final do arquivo
      arquivo->posicaoEscrita = 0;  // Reinicializa a posição de escrita para o novo bloco
      dir[arquivo->dirIndex].size += CLUSTERSIZE;  // Atualiza o tamanho do arquivo no diretório
    }
  }

  // Marca o bloco final como o último bloco usado (valor 2 na FAT)
  fat[arquivo->fim] = 2;

  // Escreve as atualizações no disco
  escreve_disco();
  escreve_dir_disco();

  return size;  // Retorna o número de bytes que foram escritos
}

int fs_read(char *buffer, int size, int file) {
  // Obtém a estrutura de arquivos abertos para o arquivo fornecido
  arquivosAbertos *arquivo = &listaArquivos[file];

  // Verifica se o arquivo está aberto no modo de leitura e se está realmente em uso
  if (arquivo->categoria != FS_R || !arquivo->ocupado) {
    printf("Erro! ");  // Mostra uma mensagem de erro se o arquivo não está em modo leitura ou não está aberto
    return -1;          // Retorna -1 indicando erro
  }

  int lidos = 0;          // Variável que conta quantos bytes foram lidos
  int blocoAtual = arquivo->primeiro;  // Define o bloco atual como o primeiro bloco do arquivo

  // Loop para ler até "size" bytes ou até o total de bytes lidos ser igual ao tamanho do arquivo
  for (int i = 0; i < size && arquivo->totalLido < dir[arquivo->dirIndex].size; i++) {
    buffer[i] = arquivo->memoria[arquivo->posicaoLeitura++];  // Lê um byte do buffer para o destino
    arquivo->totalLido++;  // Atualiza o total de bytes lidos

    // Se o índice de leitura atingiu o tamanho máximo do bloco, carrega o próximo bloco
    if (arquivo->posicaoLeitura == CLUSTERSIZE) {
      blocoAtual = fat[blocoAtual];  // Acessa o próximo bloco através da FAT (File Allocation Table)
      bl_read(blocoAtual, arquivo->memoria);  // Lê o conteúdo do novo bloco para o buffer
      arquivo->posicaoLeitura = 0;  // Reinicializa o índice de leitura para o novo bloco
    }

    lidos++;  // Incrementa o contador de bytes lidos
  }

  return lidos;  // Retorna o número total de bytes lidos com sucesso
}
