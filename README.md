# APS 2 de Computação Embarcada (24.1)

## Bruno Zalcberg e Pedro Stanzani de Freitas

Nessa atividade, fizemos um controle de *videogame* *bluetooth* dedicado ao jogo **Hi-Fi Rush**, desenvolvido pela *Tango Gameworks* e publicado pela *Bethesda*. O jogo acompanha o protagonista Chai, um garoto com o sonho de se tornar um *rockstar*. Ao se fundir com um MP3, Chai ganha poderes musicais e precisa enfrentar diversos inimigos em fases distintas.

Abaixo você confere o diagrama em blocos referente à construção e funcionamento do controle:

inserir imagem

Vovê pode conferir o vídeo de demonstração do controle [aqui](https://www.youtube.com/watch?v=WvBX_weRk6g).

### Conceitos avaliativos

#### Básicos

- [x] Desenvolvido para um jogo específico
- [x] Possui um protótipo mecânico customizado
- [x] Faz uso de RTOS
- [x] Utiliza comunicação *bluetooth* e informa se está conectado através de um LED
- [x] Possui 2x entradas analógicas e 4x entradas digitais

#### Extras

- [ ] Possui *haptic feedback*
- [ ] Utiliza ADC e IMU
- [x] O jogo deve ser "jogável" com o controle (sem latência)
- [ ] Controle recebe informações do PC/jogo e faz alguma coisa
- [ ] Hardware integrado no controle
- [x] Utiliza algum componente não visto em aula (display OLED 128x32)
- [ ] Botão de macro
- [x] Envia com a entrega vídeo TOP/stonks do controle nível quickstarter
- [x] Organiza código em arquivos `.c` e `.h`
