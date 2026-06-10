#  Dragon-01 — Sistema de Telemetria Espacial

> **Instituição:** FIAP — Faculdade de Informática e Administração Paulista  
> **Disciplina:** Edge Computing & Computer Systems  
> **Tema:** Global Solution 2026 — Indústria Espacial: O Código que Move o Universo  
> **Curso:** Engenharia de Software — 1º Ano

---

##  Integrantes

| Nome Completo | RM |
|---|---|
| Helton Pacheco dos Santos       | RM 567113 |
| Wenderson da Silva Santos       | RM 567847 |
| Geovanna Caroline Lima Santos   | RM 567754 |
| Douglas Taveira Vilella Roberto | RM 567846 |

---

##  Descrição da Solução

O projeto **Dragon-01** é um sistema de telemetria em tempo real que simula o monitoramento da cápsula **SpaceX Dragon** durante uma missão espacial. Usando um **ESP32** conectado ao broker **HiveMQ Cloud** via protocolo **MQTT**, o sistema coleta dados de três sensores — temperatura, pressão e oxigênio — e os transmite para o back-end **Node-RED**, onde os dados são processados, alertas são identificados e os valores são exibidos em gauges visuais.

Quando algum parâmetro sai da faixa nominal, o sistema dispara alertas visuais (LED) e sonoros (buzzer) a bordo da cápsula, e o Node-RED registra e exibe o alerta no painel.

O projeto se conecta ao **ODS 9 (Inovação e Infraestrutura)** ao demonstrar como tecnologias de Edge Computing podem ser aplicadas em sistemas críticos de missão espacial.

---

##  Componentes e Sensores

| Componente | Pino | Função | Faixa Nominal |
|---|---|---|---|
| DHT22 | GPIO 15 | Temperatura | 18°C a 28°C |
| Potenciômetro | GPIO 34 | Pressão | 90 a 110 kPa |
| Potenciômetro | GPIO 35 | Oxigênio | 19% a 23% |
| LED vermelho | GPIO 2 | Alerta visual | Liga em anomalia |
| Buzzer | GPIO 4 | Alerta sonoro | Bip de 150ms em anomalia |

### Como funcionam os potenciômetros

Cada potenciômetro está dividido em duas metades:

```
|←—— ESQUERDA ——→|←—— DIREITA ——→|
      NOMINAL         ANOMALIA
```

- **Metade esquerda (ADC 0 a 2047):** gera valores dentro da faixa nominal
- **Metade direita (ADC 2048 a 4095):** gera valores fora da faixa (anomalia)

---

##  Lógica de Alertas

```
SE temperatura < 18°C  OU  temperatura > 28°C  → ALERTA
SE pressão     < 90kPa OU  pressão     > 110kPa → ALERTA
SE oxigênio    < 19%   OU  oxigênio    > 23%    → ALERTA

EM CASO DE ALERTA:
  → LED (GPIO 2) acende
  → Buzzer (GPIO 4) emite bip de 150ms
  → Publica no tópico spacex/dragon01/alerts
  → Node-RED exibe alerta no painel
```

---

##  Tópicos MQTT

| Tópico | Descrição |
|---|---|
| `spacex/dragon01/telemetry` | Dados dos 3 sensores a cada 5 segundos |
| `spacex/dragon01/alerts` | Publicado quando há anomalia |

### Exemplo de payload — Telemetria

```json
{
  "temperatura": 23.4,
  "pressao": 101.0,
  "oxigenio": 21.2,
  "alerta": false
}
```

### Exemplo de payload — Alerta

```json
{
  "status": "ALERTA"
}
```

---

##  Back-end Node-RED

O Node-RED é o back-end do sistema, responsável por receber, processar e exibir os dados enviados pelo ESP32 via HiveMQ.

### O que o fluxo faz

```
[Recebe Telemetria MQTT]
        ↓
[Processa Dados]
   ↓           ↓            ↓             ↓
[Debug]   [Tem Alerta?]  [Gauge Temp]  [Gauge Pressão]  [Gauge O2]
              ↓
   [Formata Alerta] ou [Formata Nominal]
              ↓
         [Debug Status]

[Recebe Alertas MQTT] → [Debug Alerta]
```

### Como instalar o Node-RED

```bash
npm install -g node-red
```

### Como executar

```bash
node-red
```

### Como importar o fluxo

1. Abra o Node-RED 
2. Clique no menu **☰** (canto superior direito)
3. Clique em **Import**
4. Cole o conteúdo do arquivo `backend/dragon_nodered_flow.json`
5. Clique em **Import**
6. Clique em **Deploy**

### Como instalar o dashboard (gauges)

1. Menu **☰** → **Manage Palette**
2. Aba **Install**
3. Pesquise `node-red-dashboard`
4. Clique em **Install**
5. Clique em **Deploy** novamente

O painel com os gauges fica disponível em `http://localhost:1880/ui`

---


##  Como Executar

### 1. Simulação no Wokwi

1. Acesse o link público do projeto Wokwi: (https://wokwi.com/projects/466233793732245505)
2. Clique em **▶ Play**
3. Acompanhe os dados no Serial Monitor
4. Gire os potenciômetros para a direita para simular anomalias

### 2. Back-end Node-RED

1. Execute `node-red` no terminal
2. Importe o fluxo `backend/dragon_nodered_flow.json`
3. Clique em **Deploy**
4. Acesse `http://localhost:1880/ui` para ver os gauges em tempo real

### 3. Monitorar no HiveMQ

1. Acesse [HiveMQ Web Client](https://www.hivemq.com/demos/websocket-client/)
2. Conecte com as credenciais do projeto
3. Inscreva-se nos tópicos `spacex/dragon01/telemetry` e `spacex/dragon01/alerts`

---

##  Links

| Recurso | Link |
|---|---|
| Wokwi Público | https://wokwi.com/projects/466233793732245505 |
| Repositório GitHub | https://github.com/Helton-Pacheco1/GlobalSolutionESPS |

---

##  Conexão com ODS

Este projeto se conecta ao **ODS 9 — Indústria, Inovação e Infraestrutura**, demonstrando como tecnologias de Edge Computing com protocolos leves (MQTT) podem ser aplicadas em sistemas de missão crítica, como o monitoramento de cápsulas espaciais tripuladas.

---

*FIAP — Edge Computing & Computer Systems · Global Solution 2026*
