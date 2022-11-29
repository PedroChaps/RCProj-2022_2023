# Perguntar

[X] - "GSport". O que acontece se a porta do outro lado não estiver aberta?
[X] - Podemos usar um buffer com tamanho grande para não nos preocuparmos com Mallocs (para UDP só. Para TCP usamos buffer vamos preenchendo, enviando, preenchendo)
[X] - Em UDP, se enviarmos uma letra mas o SV não responder, é suposto enviarmos denovo? Temos tipo um timeout que vai enviando?
[X] - Temos que ir sempre pondo os comandos "SNG", "RSG", "PLG", ...
[ ] - Podemos assumir que o input vem sempre bem formado? As in, não nos temos que preocupar com espaços a mais, validar que um IP é mesmo um IP, o PLID está entre ... e ..., etc.

# TODOs 

[ ] - Fazer download do enunciado denovo
[ ] - Quando cliente faz "Create new game" e o sv não responde, temos que ter um timer que faz isso mais uma vez ou outra. Se ao fim dessas vezes o servidor não responder, informamos que o sv não respondeu
[ ] - Ver Slides fixes 23-31... aula 3. Explicam servidores TCP concorrentes