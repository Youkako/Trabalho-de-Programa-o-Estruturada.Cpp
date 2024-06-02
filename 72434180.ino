#include <MD_MAX72xx.h>//para o controle da matriz de leds.
#include <SPI.h>//biblioteca para a comunicação

//Define várias constantes usadas para controlar o comportamento do código.
#define IMMEDIATE_NEW   0
#define USE_POT_CONTROL 1
#define PRINT_CALLBACK  0

//HARDWARE_TYPE e MAX_DEVICES especificam o tipo de hardware e o número de dispositivos na cadeia.
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 11
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }
//CLK_PIN, DATA_PIN e CS_PIN são os pinos usados para a comunicação SPI com a matriz de LEDs.
#define CLK_PIN   18  // or SCK
#define DATA_PIN  23  // or MOSI
#define CS_PIN    5

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
//Inicializa a matriz de LEDs com o tipo de hardware, o pino de chip select (CS) e o número de dispositivos

#if USE_POT_CONTROL //Define parâmetros para o controle da velocidade de rolagem, espaçamento entre caracteres e tamanhos dos buffers de mensagens.
#define SPEED_IN  A5
#else
#define SCROLL_DELAY  75
#endif
#define CHAR_SPACING  1
#define BUF_SIZE  75

//curMessage contém a mensagem atual a ser rolada.
uint8_t curMessage[BUF_SIZE] = { "     CAINA E UM BUNDA MOLE!      "};
//newMessage é um buffer para uma nova mensagem recebida.
uint8_t newMessage[BUF_SIZE];
//newMessageAvailable é uma flag para indicar a disponibilidade de uma nova mensagem.
bool newMessageAvailable = false;

uint16_t scrollDelay;

//Lê dados da interface serial e armazena-os em newMessage. Quando uma nova linha é detectada ou o buffer está cheio, a mensagem é marcada como disponível.
void readSerial(void)
{
  static uint8_t putIndex = 0;

  while (Serial.available())
  {
    newMessage[putIndex] = (char)Serial.read();
    if ((newMessage[putIndex] == '\n') || (putIndex >= BUF_SIZE-3))
    {
      newMessage[putIndex++] = ' ';
      newMessage[putIndex] = '\0';
      putIndex = 0;
      newMessageAvailable = true;
    }
    else if (newMessage[putIndex] != '\r')
      putIndex++;
  }
}

//Callback para dados que estão sendo rolados para fora da tela. Não faz nada significativo a menos que PRINT_CALLBACK seja ativado.
void scrollDataSink(uint8_t dev, MD_MAX72XX::transformType_t t, uint8_t col) {} 

//Controla o fluxo de dados para a rolagem da mensagem na tela. Utiliza uma máquina de estados para determinar o que deve ser mostrado em cada coluna da matriz de LEDs.
uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t)
{
  static uint8_t* p = curMessage;
  static enum { NEW_MESSAGE, LOAD_CHAR, SHOW_CHAR, BETWEEN_CHAR } state = LOAD_CHAR;
  static uint8_t curLen, showLen;
  static uint8_t cBuf[15];
  uint8_t colData = 0;

#if IMMEDIATE_NEW
  if (newMessageAvailable)
  {
    state = NEW_MESSAGE;
    mx.clear();
  }
#endif

  switch(state)
  {
    case NEW_MESSAGE:
      memcpy(curMessage, newMessage, BUF_SIZE);
      newMessageAvailable = false;
      p = curMessage;
      state = LOAD_CHAR;
      break;

    case LOAD_CHAR:
      showLen = mx.getChar(*p++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state = SHOW_CHAR;
      if (*p == '\0')
      {
        p = curMessage;
#if !IMMEDIATE_NEW
        if (newMessageAvailable)
        {
          state = NEW_MESSAGE;
          break;
        }
#endif
      }

    case SHOW_CHAR:
      colData = cBuf[curLen++];
      if (curLen == showLen)
      {
        showLen = CHAR_SPACING;
        curLen = 0;
        state = BETWEEN_CHAR;
      }
      break;

    case BETWEEN_CHAR:
      colData = 0;
      curLen++;
      if (curLen == showLen)
        state = LOAD_CHAR;
      break;

    default:
      state = LOAD_CHAR;
  }

  return colData;
}

//Faz a rolagem do texto na tela baseado no intervalo de tempo definido por scrollDelay.
void scrollText(void)
{
  static uint32_t prevTime = 0;

  if (millis() - prevTime >= scrollDelay)
  {
    mx.transform(MD_MAX72XX::TSL);
    prevTime = millis();
  }
}


//Determina a velocidade de rolagem com base na leitura de um potenciômetro ou em um valor fixo.
uint16_t getScrollDelay(void)
{
#if USE_POT_CONTROL
  uint16_t t;
  t = analogRead(SPEED_IN);
  t = map(t, 0, 1023, 25, 250);
  return t;
#else
  return SCROLL_DELAY;
#endif
}

//Inicializa a matriz de LEDs, configura os callbacks e a comunicação serial, e exibe uma mensagem de instrução na serial.
void setup()
{
  mx.begin();
  mx.setShiftDataInCallback(scrollDataSource);
  mx.setShiftDataOutCallback(scrollDataSink);

#if USE_POT_CONTROL
  pinMode(SPEED_IN, INPUT);
#else
  scrollDelay = SCROLL_DELAY;
#endif

  newMessage[0] = '\0';

  Serial.begin(57600);
  Serial.print("\n[MD_MAX72XX Message Display]\nType a message for the scrolling display\nEnd message line with a newline");
}

//No loop principal, atualiza a velocidade de rolagem, lê novas mensagens da serial e faz a rolagem do texto na tela.
void loop()
{
  scrollDelay = getScrollDelay();
  readSerial();
  scrollText();
}





