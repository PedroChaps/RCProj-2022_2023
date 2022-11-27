# Tradução do enunciado para TODOs
A ideia é ir preenchendo as "checkboxes" quando implementarmos cada feature

## Player

[X] - Variável global GN

[X] - Cada player tem um PLID, string com 6 digitos

[ ] - É começado assim: `./player [-n GSIP] [-p GSport]`

[ ] - verificar que GSIP é um ip válido
[ ] - Se GSIP omitido, meter que é a máquina local (127.0.0.1 I guess)

[ ] - verificar que GSport é válida
[ ] - Se GSport omitida, meter o default = 58000+GN

### Start

[ ] - Comando `start PLID` ou `sg PLID` - UDP
      
### Play
[ ] - ...

## GS

[ ] - Cada player tem um PLID, string com 6 digitos

[ ] - Começar jogo novo - The GS randomly selects a word from its word_file. Words are in English 
      and can have between 3 and 30 letters. For playing there is no distinction
      between the usage of lowercase or uppercase letters.
[ ] - The GS also sets the maximum number of errors that can be committed before losing the game, 
       max_errors, according to the selected word length:
       > max_errors = 7, for word lengths up to 6 letters
       > max_errors = 8, for word lengths between 7 and 10 letters
       > max_errors = 9, for word lengths of 11 letters or more


## Escrita em ficheiros

### Diretoria GAMES (ver exemplo em `RC_GUIAPROJ_2223_23NOV2022.pdf`)

[ ] - Os ficheiros têm o formato `game_(plid).txt`

[ ] - A primeira linha contém a palavra a decifrar seleccionada pelo GS para o jogo em curso e a 
      designação do ficheiro de ajuda a enviar para o player quando este emite o comando ’hint’

[ ] - Cada linha deste ficheiro a partir da segunda linha inclusiv´e, refere-se a uma jogada. 
      Cadajogada é caracterizada pela variável ’code’ e pela jogada ’play’. Quando a variável ’code’ tem o valor T (Trial) significa que ’play’ é uma letra enviada ao GS pelo player na sequência de um comando ’play’. Quando a variável ’code’ tem o valor G (Guess) significa que ’play’ é uma palavra enviada ao GS pelo player na sequência de um comando ’guess’.
      
[ ] - Apenas as jogadas novas são armazenadas neste ficheiro. A uma jogada repetida o GS responde 
      com status==DUP não registando a jogada no ficheiro game_(plid).txt.

[ ] - Quando um jogo termina, ele pode terminar com sucesso, insucesso ou desistência. 
      Quando um jogo termina, o ficheiro GAME (plid).txt é transferido para a directoria do player em causa, directoria essa que é criada sob a directoria games/finished (com a designação do PLID) quando termina o primeiro jogo do referido player. 
      
      Na tranferência do jogo finalizado por um dado player o ficheiro game_(plid).txt assume uma nova designação que facilita o seu tratamento em posteriores operações.
      O nome do ficheiro passa a conter em primeiro lugar a data de finalização seguida da hora de finalização, seguida esta última de um código de um caracter que indica se o jogo terminou com sucesso (W de Win), insucesso (F de Fail) ou desistência (Q de Quit) de acordo com o formato: YYYYMMDD_HHMMSS_code.txt

### Diretoria SCORES

[ ] - Quando um dado jogo termina com sucesso, é também criado um ficheiro de ’score’ na directoria 
      SCORES. A designação do ficheiro de ’score’ é: `score_PLID_DDMMYYYY_HHMMSS.txt`, em que `score` tem um valor [001 - 100] sempre com três digitos e PLID é a identificação do jogador. `DDMMYYYY` e `HHMMSS` são a data e a hora a que o jogo terminou com sucesso respectivamente.
       (A designação acima facilita a obtenção da lista de ’scores’ ordenados por valor decrescente de ’score’.)
       
[ ] - Cada ficheiro de score tem uma única linha com o seguinte formato: 
      `score PLID palavra n_succ n_trials` em que `n_succ` e `n_trials` são o número de tentativas bem sucedidas e o número total de tentativas, respectivamente.

### Encontrar último jogo

[ ] - Usar a função `FindLastGame()` para, dado um PLID, encontrar o jogo mais recente:
    ```c
    #include <dirent.h>

    int FindLastGame(char *PLID, char *fname)
    {
        struct dirent **filelist;
        int n_entries, found;
        char dirname[20];
        sprintf(dirname, "GAMES/%s/", PLID);

        n_entries = scandir(dirname, &filelist, 0, alphasort);
        found = 0;
        if (n_entries <= 0)
            return (0);
        else
        {
            while (n_entries--)
            {
                if (filelist[n_entries]->d_name[0] != '.')
                {
                    sprintf(fname, "GAMES/%s/%s", PLID, filelist[n_entries]-> d_name);
                    found = 1;
                }
                free(filelist[n_entries]);
                if (found)
                    break;
            }
            free(filelist);
        }
        return (found);
    }
    ```
    
[ ] - Usar a função `FindTopScores()` para ter acesso a cada um dos 10 ficheiros com scores mais
      elevados contidos na directoria SCORES
      ```c
      #include <stdio.h>
      #include <dirent.h>
  
      int FindTopScores(SCORELIST **list)
      {
          struct dirent **filelist;
          int n_entries, i_file;
          char fname[50];
          FILE **fp;
          n_entries = scandir("SCORES/", &filelist, 0, alphasort);
          i_file = 0;
          if (n_entries < 0)
          {
              return (0);
          }
          else
          {
              while (n_entries--)
              {
                  if (filelist[n_entries]->d_name[0] != '.')
                  {
                      sprintf(fname, "SCORES/%s", filelist[n_entries]->d_name);
                      fp = fopen(fname, "r");
                      if (fp != NULL)
                      {
                          fscanf(fp, "%d%s%s%d%d",
                              &list->score[i_file], list->PLID[i_file], list->word[i_file], &list->nsucc[i_file],
                              &list->ntot[i_file]);
                          fclose(fp);
                          ++i_file;
                      }
                  }
                  free(filelist[n_entries]);
                  if (i_file == 10)
                      break;
              }
              free(filelist);
          }
          list->n_scores = i_file;
          return (i_file);
      }
      ```
 


## Geral

[ ] - As chamadas de sistema read() e write() podem ler e escrever, respetivamente, um numero de 
      bytes inferior ao que lhes foi solicitado – deve garantir que ainda assim a sua implementação funciona corretamente. 

[ ] - Os processos (clientes e servidores) não devem terminar abruptamente. Por exemplo: 
      Se servidor receber mensagens mal formatadas: responde com mensagem de erro apropriada, como definido no protocolo.
      Se cliente receber mensagens mal formatadas: temina interação com o servidor e informa o utilizador imprimindo uma mensagem de erro no écran.
      Erros de chamadas de sistema: as aplicações não devem terminar catastroficamente, evitando observar-se mensagens de erro do sistema operativo tais como "segmentation fault" ou "core dump"."
