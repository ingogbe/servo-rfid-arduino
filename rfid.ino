//Incluir biblioteca RFID de https://github.com/song940/RFID-RC522
#include <RFID.h>
//Incluir biblioteca padrão para o Servo Motor
#include <Servo.h>

// Declaramos o pino SDA do Arduino para usar no sensor RFID
#define SS_PIN 10
// Declaramos o pino RST do Arduino para usar no sensor RFID
#define RST_PIN 9

//Declaramos os valores do pinos do led RGB
#define RED 4
#define GREEN 3
#define BLUE 2

//Iniciamos o objeto RFID, passando os pinos acima
RFID rfid(SS_PIN, RST_PIN); 

//Criamos uma instância do objecto de controle do Servo Motor
Servo myservo;        

//Declaramos a variavel que vai salvar a posição atual do Servo Motor e tempo de espera até fechar o portão
int pos = 0;
int closeTime = 2000;

//Definição da estrutura utilizada para armazenas os dados do RFID
typedef struct {
  int number;
  float balance;
} RfidItem;

//Declaração do array dos RFIDs cadastrados no sistema
const int registeredItemsSize = 1;
RfidItem registeredItems[registeredItemsSize] = {
  { .number = 576, .balance = 20 }, 
};

//Declaração das variaveis relevantes do estacionamento
const int parkingSpace = 10;
float parkPrice = 5.40;
int parkedItems[parkingSpace];
int currentParkedItems = 0;

 
void setup() {
  //Iniciamos a comunicação serie para ler as respostas do módulo
  Serial.begin(9600); 
  //Iniciamos a comunicação SPI
  SPI.begin();    
  //Iniciamos o objeto RFID declarado anteriormente    
  rfid.init();    

  //Configura os pinos do led como saída
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  //Configura o Servo Motor no pino 9
  myservo.attach(9);  
  //Coloca o Servo Motor na posição 0
  myservo.write(0);

  //Inicializando estacionamento vazio;
  for(int i = 0; i < parkingSpace; i++){
    parkedItems[i] = -1;
  }

  //Liga o led azul
  turnOnBlue();
}
 
//Na função loop fazemos a checagem de leitura do cartão e a cada leitura é executado o conjunto de instruções pertinentes
void loop() {
  check_for_card();
}

//Desliga todos os leds e liga somente o vermelho
void turnOnRed(){
  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}

//Desliga todos os leds e liga somente o azul
void turnOnBlue(){
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, HIGH);
}

//Desliga todos os leds e liga somente o verde
void turnOnGreen(){
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, HIGH);
  digitalWrite(BLUE, LOW);
}

//Entra ou sai do estacionamento
void parkItem(int number){
  int found = 0;

  //Verifica se o estacionamento esta cheio ou não
  //Se estiver manda mensagem que esta cheio e liga o led vermelho para indicar
  if(currentParkedItems == parkingSpace){
    Serial.println("Park is full!");
    turnOnRed();
    delay(1000);
    turnOnBlue();
  }
  else {
    //Busca se o RFID lido está cadastrado
    //Se não estiver não entra no IF e a variavel "found" continua em 0
    for(int i = 0; i < registeredItemsSize; i++){
      if(registeredItems[i].number == number){
        found = 1;
        
        int inside = 0;

        //Verifica se o RFID está saindo ou entrando do estacionamento
        for(int j = 0; j < parkingSpace; j++){
          if(parkedItems[j] == number){
            //Se estiver saindo, liga o led verde, manda mensagem de tchau e mostra o crédito atual do cartão
            //Remove o item do array do estacionamento e reduz a variavel q faz controle se esta cheio ou não
            //Abre e fecha o portão, e após tudo isso led volta a ser azul (esperando)
            turnOnGreen();
            Serial.println("Good Bye!");
            Serial.print("Current balance: "); Serial.println(registeredItems[i].balance);
            parkedItems[j] = -1;
            currentParkedItems--;
            inside = 1;
            openGate();
            turnOnBlue();
            break;
          }
        }

        if(inside == 0){
          //Se não estiver dentro do estacionamento, ele liga o led verde
          //mostra mensagem de boas vindas, desconta o valor do crédito do cartão
          //adiciona o item na lista de items que estão no estacionamento
          //abre e fecha o portão e após tudo liga o led azul novamente
          if(registeredItems[i].balance >= parkPrice){
            turnOnGreen();

            Serial.println("Please enter the park!");
            registeredItems[i].balance -= parkPrice;
            Serial.print("Current balance: "); Serial.println(registeredItems[i].balance);

            for(int j = 0; j < parkingSpace; j++){
              if(parkedItems[j] == -1){
                parkedItems[j] = number;
                currentParkedItems++;
                break;
              }
            }

            openGate();
            turnOnBlue();
          }
          else{
            //Se não houver crédito suficente no cartão, liga o led vermelho e mostra mensagem
            //dizendo que não há crédito suficiente, após 1s liga led azul novamente
            turnOnRed();
            Serial.println("You do not have enough money in your card!");
            Serial.print("Current balance: "); Serial.println(registeredItems[i].balance);
            delay(1000);
            turnOnBlue();
          }
        }

        break;
      }
    }

    //Se não encontrou o cartão na lista de registrados
    //mostra mensagem liga o led vermelhor e após 1s liga o led azul novamente
    if(found == 0){
      Serial.println("Your card is not registered!");
      turnOnRed();
      delay(1000);
      turnOnBlue();
    }
  }
}
 
void check_for_card(){
  //Faz leitura do cartão
  if (rfid.isCard()) {
    Serial.println("Find the card!");
    //Se a leitura é bem sucedida chama a função para iniciar a entrada ou saida no estacionamento.
    //Leitura do numero de serie do cartão de 4 bytes
    if (rfid.readCardSerial()) {
      //Mostra numero do cartão
      Serial.print("The card's number is: ");
      Serial.println(rfid.serNum[0] + rfid.serNum[1] + rfid.serNum[2] + rfid.serNum[3] + rfid.serNum[4]);

      /*
      Serial.print(rfid.serNum[0], HEX); Serial.print(" ");
      Serial.print(rfid.serNum[1], HEX); Serial.print(" ");
      Serial.print(rfid.serNum[2], HEX); Serial.print(" ");
      Serial.print(rfid.serNum[3], HEX); Serial.print(" ");
      Serial.print(rfid.serNum[4], HEX); Serial.print(" ");

      Serial.println("");
      Serial.print(rfid.serNum[0], BIN); Serial.print(" ");
      Serial.print(rfid.serNum[1], BIN); Serial.print(" ");
      Serial.print(rfid.serNum[2], BIN); Serial.print(" ");
      Serial.print(rfid.serNum[3], BIN); Serial.print(" ");
      Serial.print(rfid.serNum[4], BIN); Serial.print(" ");

      Serial.println("");
      Serial.print(rfid.serNum[0], DEC); Serial.print(" ");
      Serial.print(rfid.serNum[1], DEC); Serial.print(" ");
      Serial.print(rfid.serNum[2], DEC); Serial.print(" ");
      Serial.print(rfid.serNum[3], DEC); Serial.print(" ");
      Serial.print(rfid.serNum[4], DEC); Serial.print(" = ");
      Serial.println(rfid.serNum[0] + rfid.serNum[1] + rfid.serNum[2] + rfid.serNum[3] + rfid.serNum[4]);
      */

      parkItem(rfid.serNum[0] + rfid.serNum[1] + rfid.serNum[2] + rfid.serNum[3] + rfid.serNum[4]);
      Serial.println("--------------------------");
    }
  }

  //Espera nova leitura
  rfid.halt();
}

//Abre o portão (servo motor, em 90 graus)
void openGate(){
  Serial.println("Opening gate");
  //Gira o servo motor do grau 0 (inicial) ao 90 de grau em grau com delay de 15ms (para garatir funcionamento
  //correto, praticamente espera o servo motor fazer o deslocamento por 15ms para então fazer o proximo)
  for (pos = 0; pos <= 90; pos += 1) {
    // in steps of 1 degree
    myservo.write(pos);
    delay(15);
  }

  //Espera um certo tempo e então fecha o portão
  delay(closeTime);  
  closeGate();
}

void closeGate(){
  Serial.println("Closing gate");
  //Gira o servo motor do grau 90 (final) ao 0 de grau em grau com delay de 15ms (para garatir funcionamento
  //correto, praticamente espera o servo motor fazer o deslocamento por 15ms para então fazer o proximo)
  for (pos = 90; pos >= 0; pos -= 1) {
    myservo.write(pos);
    delay(15);
    }
}
