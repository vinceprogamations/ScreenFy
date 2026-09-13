# Arquitetura e Especificação de Engenharia para Partilha de Ecrã de Ultra-Baixa Latência (4K 120 FPS) via Radmin VPN

A viabilização de uma plataforma dedicada à transmissão e partilha de ecrã em tempo real a uma resolução de $3840 \times 2160$ pixeis (4K) com cadências de 60 ou 120 fotogramas por segundo (FPS) coloca desafios físicos e computacionais extremos aos subsistemas de captura do sistema operativo, à aceleração de vídeo por hardware e à infraestrutura de rede. Diferenciando-se de soluções genéricas de conferência como o Discord — cujo pipeline privilegia a economia de largura de banda e introduz múltiplos estágios de amortecimento (buffering) que elevam a latência para a ordem dos $80\text{--}150\text{ ms}$ —, a transmissão interativa de elevado débito exige uma arquitetura estritamente decalcada dos sistemas de computação gráfica de alto rendimento, tais como os motores Sunshine e o protocolo GameStream.

A escolha da plataforma Radmin VPN como vetor de interligação estabelece uma rede de área local virtual de Camada 2/3 (L2/L3) através da Internet pública. Esta topologia elimina a complexidade dos protocolos de descoberta e travessia de NAT (Network Address Translation) mediante servidores de retransmissão externos (TURN), mas introduz restrições severas ao nível da Unidade Máxima de Transmissão (Maximum Transmission Unit - MTU) e da fragmentação de pacotes de dados no protocolo UDP.

A concretização de uma infraestrutura deste calibre exige uma especificação técnica modular e exaustiva, desenhada para ser interpretada e executada de forma determinística por agentes autónomos de geração de código, tais como Claude Code, OpenAI Codex ou Antigravity. O presente documento define a análise física dos componentes, o desenho arquitetural em quatro etapas e a correspondente decomposição em quatro lotes atómicos de implementação.

---

## 1. Análise Física dos Componentes

O pipeline de vídeo de ultra-baixa latência é medido na ordem dos sub-milissegundos por etapa. A transmissão a 120 FPS exige que cada frame seja capturado, codificado, transmitido, descodificado e apresentado em intervalos ideais de $\sim8.33\text{ ms}$.

### 1.1 Captura de Ecrã (Screen Capture)
A captura convencional impõe cópias sistémicas (RAM para VRAM e VRAM para RAM), introduzindo latência inaceitável. O sistema deve interagir diretamente com o Desktop Window Manager (DWM) da Microsoft via **DXGI Desktop Duplication API**, garantindo uma captura *Zero-Copy*. Os frames residirão exclusivamente na memória de vídeo (VRAM) como texturas Direct3D 11, prontos a ser alimentados diretamente ao codificador de hardware.

### 1.2 Codificação de Vídeo (Video Encoding)
A compressão bruta de 4K a 120 FPS não comprimida requer cerca de 24 Gbps, inviável via Internet. A utilização de aceleração por hardware (NVENC para NVIDIA, AMF para AMD, ou QuickSync para Intel) é mandatória.
*   **Codec:** HEVC (H.265) ou AV1 para maximizar a qualidade visual versus rácio de compressão.
*   **Tuning de Baixa Latência:** O codificador operará no modo *Ultra-Low Latency* (ULL):
    *   Eliminação de frames bidirecionais (B-Frames) para impedir reordenação.
    *   Tamanho de Group of Pictures (GOP) infinito (somente intra-frame inicial).
    *   Controlo de rácio (Rate Control) estrito de tipo CBR (Constant Bitrate) acoplado ativamente com a telemetria da rede.

### 1.3 Rede e Transporte (Network constraints via Radmin VPN)
A Radmin VPN atua como o túnel virtual, cifrando os pacotes, o que reduz a MTU efetiva da rede subjacente.
*   **Protocolo:** UDP é estritamente obrigatório. O protocolo TCP introduziria retransmissões automáticas (*Head-of-Line Blocking*), violando os requisitos de tempo real estrito.
*   **Packetization:** O stream de pacotes do codificador (NAL units - Network Abstraction Layer) deve ser deliberadamente fragmentado pela nossa camada de rede para assegurar que cada datagrama UDP, após a encapsulação Radmin VPN (tipicamente IPsec/OpenVPN overhead), se mantenha inferior à MTU efetiva (geralmente fixada conservadoramente nos $1350\text{ bytes}$) para impedir fragmentação invisível a nível de IP, que destrói a fiabilidade.
*   **Correção de Erros (Forward Error Correction - FEC):** Para mitigar perdas de pacotes inerentes à internet pública sem invocar retransmissões caras, empregar-se-á um esquema de paridade Reed-Solomon ajustado dinamicamente para redundância de até 10%.

### 1.4 Descodificação e Apresentação
No cliente recetor, os datagramas UDP são reconstruídos no pipeline e alimentados diretamente ao descodificador de hardware via interfaces como **D3D11VA** (Direct3D 11 Video Acceleration). O buffer de jitter será minimizado para um único frame. A apresentação far-se-á através da DXGI SwapChain em modo *Flip Discard*, contornando o compositor de ambiente de trabalho (DWM) sempre que possível para suprimir o V-Sync lag.

---

## 2. Fundamentos de Desempenho e Engenharia do Pipeline de Vídeo

O princípio basilar que dita o sucesso de uma aplicação de transmissão em tempo real reside no respeito estrito pelo orçamento temporal por fotograma (*frame time budget*). A uma cadência de 120 FPS, o intervalo disponível para a totalidade das operações de processamento entre fotogramas sucessivos é de rigorosamente $8,333\text{ ms}$, enquanto a 60 FPS esse limiar se fixa nos $16,667\text{ ms}$. Cada etapa individual — desde a aquisição da superfície gráfica no monitor do hospedeiro (*host*) até à respetiva renderização no monitor do recetor (*client*) — deve operar de forma síncrona ou em pipelines assíncronos sem contenção de bloqueio (*non-blocking lock-free queues*).

### 2.1 Orçamento Temporal e Fluxo de Dados Brutos

O processamento de vídeo em 4K não comprimido inviabiliza terminantemente qualquer transferência de fotogramas através da memória principal do sistema (RAM do processador). O volume de dados gerado por um único fotograma descompactado em formato matricial de cor BGRA a 32 bits por pixel (4 bytes por pixel) na resolução nativa de $3840 \times 2160$ pixeis totaliza $33.177.600\text{ bytes}$ (aproximadamente $31,64\text{ MB}$).

Operando a 120 fotogramas por segundo, a taxa de débito interno bruto exigida para sustentar este fluxo atinge:

$$31,64\text{ MB} \times 120\text{ FPS} = 3.796,8\text{ MB/s} \approx 3,80\text{ GB/s} \quad (30,38\text{ Gbps})$$

A tentativa de descarregar estes dados da VRAM da placa gráfica para a memória de sistema através do barramento PCIe para posterior compressão via software consumiria uma fração desproporcionada da largura de banda do barramento e adicionaria uma latência de trânsito bidirecional superior a $15\text{ ms}$, violando imediatamente o orçamento temporal do sistema.

Por conseguinte, a totalidade da cadeia de dados deve assentar numa arquitetura de cópia absolutamente nula (*zero-copy*), assegurando que o fotograma gerado permaneça em memória dedicada de vídeo (VRAM) desde a sua captura na interface de apresentação gráfica até à sua receção direta pelo bloco físico de codificação de hardware.

**Tabela: Subsistema de Processamento e Orçamento Temporal**

| Subsistema de Processamento | Alvo Temporal (4K 60 FPS) | Alvo Temporal (4K 120 FPS) | Mecanismo de Implementação |
| :--- | :--- | :--- | :--- |
| Captura de Ecrã (GPU) | $\le 2,50\text{ ms}$ | $\le 1,20\text{ ms}$ | IDXGIOutputDuplication / WinRT WGC |
| Conversão de Cor (Shader) | $\le 0,80\text{ ms}$ | $\le 0,40\text{ ms}$ | Direct3D 11 Compute Shader (BGRA $\to$ NV12) |
| Codificação de Hardware | $\le 4,00\text{ ms}$ | $\le 2,40\text{ ms}$ | NVENC P1 / AMD AMF Ultra-Low Latency |
| Encapsulamento e FEC | $\le 0,60\text{ ms}$ | $\le 0,30\text{ ms}$ | Fragmentação RTP com Reed-Solomon em CPU |
| Trânsito de Rede (Radmin) | $\le 4,50\text{ ms}$ | $\le 2,00\text{ ms}$ | Sockets UDP diretos sem filas de jitter |
| Descodificação de Hardware | $\le 2,50\text{ ms}$ | $\le 1,20\text{ ms}$ | Direct3D 11 Video Acceleration (D3D11VA) |
| Apresentação Gráfica (UI) | $\le 1,50\text{ ms}$ | $\le 0,80\text{ ms}$ | DXGI Flip-Model (FLIP_DISCARD, V-Sync Off) |
| **Latência Fim-a-Fim Global** | $\le 16,40\text{ ms}$ | $\le 8,30\text{ ms}$ | **Pipeline síncrono Zero-Copy integral** |

### 2.2 Dimensionamento da Taxa de Compressão e Seleção de Codecs

O cálculo da taxa de compressão necessária para assegurar qualidade visual cristalina com fidelidade idêntica ao sinal original ancora-se na densidade de informação expressa em bits por pixel (Bits Per Pixel - BPP). O débito binário nominal é obtido pela expressão matemática:

$$\text{Bitrate (kbps)} = \frac{\text{Largura} \times \text{Altura} \times \text{Taxa de Quadros} \times \text{BPP}}{1000}$$

Para cenários interativos de elevada complexidade visual e movimentação dinâmica rápida, valores de BPP na ordem de $0,08$ a $0,10$ estabelecem o limiar no qual artefactos de compressão se tornam praticamente impercetíveis ao olho humano em monitores de alta densidade de pixeis. Fixando um valor conservador de $0,085$ para compressão baseada em perfis avançados de vídeo, obtém-se:

$$\text{Bitrate}_{\text{4K120}} = \frac{3840 \times 2160 \times 120 \times 0,085}{1000} \approx 84.579\text{ kbps} \approx 85\text{ Mbps}$$

Durante momentos de transição abrupta de ecrã ou rotações angulares rápidas de câmara em jogos tridimensionais, o fluxo de dados atinge picos de entropia que demandam margens de largura de banda na faixa dos $120\text{ a }150\text{ Mbps}$ para prevenir a macroblocagem. Sob o túnel da Radmin VPN, executado em conexões locais com fios ou redes de banda larga simétrica, estas taxas são plenamente sustentáveis, desde que o pipeline de rede mitigue a fragmentação na camada de protocolo de Internet.

No confronto analítico entre os formatos de compressão contemporâneos, a arquitetura deve priorizar o HEVC (H.265) e o AV1. Embora o AV1 exiba ganhos substanciais na velocidade de descompressão em blocos de hardware modernos — atingindo tempos médios de descodificação de $0,39\text{ ms}$ face aos $0,90\text{ ms}$ registados pelo HEVC —, o suporte universal a codificadores HEVC em praticamente todas as GPUs NVIDIA (desde a microarquitetura Pascal) e AMD confere ao HEVC a base primária de interoperabilidade. A solução ideal consiste numa seleção dinâmica: o codificador negoceia a inicialização de uma sessão AV1 caso ambos os terminais gráficos disponham de aceleração dedicada correspondente; caso contrário, estabelece a transmissão sob perfil HEVC de baixa complexidade temporal.

---

## 3. Subsistemas de Captura Gráfica, Codificação em Hardware e Áudio

A extração de fotogramas e o tratamento áudio no sistema operativo Microsoft Windows exigem o recurso direto às interfaces de programação de baixo nível do subsistema gráfico e de som.

### 3.1 Desktop Duplication versus Windows Graphics Capture

O sistema operativo Windows oferece duas interfaces principais para a interceção de ecrã sem introdução de ganchos (*hooks*) de injeção de DLL: a **Desktop Duplication API**, implementada sobre a infraestrutura DXGI, e a **Windows Graphics Capture API (WGC)**, introduzida nas compilações mais recentes do Windows 10 e Windows 11 via WinRT.

A interface `IDXGIOutputDuplication` comunica diretamente com o controlador do monitor através do pipeline do Direct3D 11, entregando os fotogramas através de superfícies `ID3D11Texture2D` com enriquecimento de metadados como caixas delimitadoras de modificação (*dirty rects*) e coordenadas exatas do cursor do rato acopladas a carimbos temporais de alta precisão baseados no registo QPC (`QueryPerformanceCounter`). No entanto, a evolução do subsistema de composição gráfica do Windows 11 (especialmente a partir da versão 24H2) alterou o funcionamento do DXGI ao torná-lo estritamente dependente de planos de sobreposição múltipla (*Multiplane Overlays* - MPO). Em configurações de adaptadores gráficos onde o suporte MPO seja deficiente, a API Desktop Duplication pode apresentar anomalias de cadência e hesitações na captura simultânea de janelas aceleradas.

Em contrapartida, a interface Windows Graphics Capture foi profundamente estabilizada pela Microsoft nas atualizações contemporâneas, solucionando constrangimentos cronológicos de captura desnecessária de fotogramas repetidos e operando harmoniosamente com superfícies de inversão direta (*DirectFlip*). A arquitetura de captura aqui delineada implementa um modelo híbrido: inicializa primariamente a captura via `IDXGIOutputDuplication` devido à sua proximidade ao controlador gráfico e à capacidade de consulta imediata com tempo de espera nulo (`AcquireNextFrame(0, ...)`), garantindo tempo de resposta mínimo; caso a chamada de sistema reporte falha persistente de sincronismo ou opere sob configurações multimonitor de adaptadores heterogéneos, o motor comuta dinamicamente para o `Direct3D11CaptureFramePool` da API WGC, preservando a entrega de fotogramas para a textura D3D11 em memória de vídeo.

### 3.2 Codificação Zero-Copy via Direct3D 11 e NVENC/AMF

Para suprimir por completo cópias intermediárias para a memória do processador, o fotograma capturado pelo DXGI ou WGC — originalmente formatado em espaço de cor BGRA com 8 bits por componente — é submetido a um passo de conversão interna na GPU mediante a execução de um Pixel Shader ou Compute Shader em Direct3D 11. O sombreador converte as matrizes de cor RGB para o formato subsamostrado NV12 (composto por um plano Y de luminância e um plano UV intercalado de crominância a 4:2:0), gerando uma segunda textura `ID3D11Texture2D` em formato `DXGI_FORMAT_NV12`.

A ligação ao bloco físico de codificação por hardware da placa gráfica (como o NVENC da NVIDIA) é efetuada registando a referida textura como recurso de entrada através da rotina nativa `nvEncRegisterResource` com o tipo de recurso `NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX`. O fluxo de controlo progride sem recurso a estruturas de ramificação de decisão complexas, processando-se de modo estritamente sequencial:

1. A superfície gráfica é adquirida pelo motor DXGI ou WGC diretamente na VRAM.
2. Em seguida, o sombreador D3D11 executa a transposição de espaço de cor para a textura mapeada em NV12.
3. O codificador NVENC efetua o bloqueio interno do recurso via chamada `nvEncMapInputResource`, codifica os macroblocos em circuitos integrados dedicados sem consumo de ciclos de computação dos núcleos CUDA, e emite o fluxo binário comprimido (NAL units) para um amortecedor circular de saída.

Para assegurar uma cadência temporal sem acumulação de atraso, o codificador NVENC é parametrizado com configurações estritas:
* O perfil de afinação é estabelecido como `NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY`, o qual suprime o escalonamento em múltiplos passos de análise visual.
* A predefinição estrutural de desempenho é fixada na diretiva `NV_ENC_PRESET_P1_GUID`, que prioriza a menor duração do ciclo de codificação em detrimento de rácios residuais de compressão.
* A estrutura do grupo de imagens (*Group of Pictures* - GOP) opera sem quaisquer fotogramas de previsão bidirecional (proibição total de B-Frames através de `frameIntervalP = 1`), eliminando a necessidade de reordenação temporal de fotogramas que introduziria pelo menos um a dois fotogramas de atraso de apresentação.
* O controlo de taxa de dados opera em modo de taxa constante estrita (CBR) com `enablePTD = 1`, e a prevenção de picos térmicos ou de rede decorrentes de fotogramas *intra* completos (*Keyframes* IDR) é mitigada através da técnica de atualização intra progressiva (*Intra-Refresh* contínuo), na qual frações verticais de fotogramas intra são distribuídas ao longo de sucessivos fotogramas intermédios.

### 3.3 Captura de Áudio de Sistema via WASAPI Loopback

A ingestão sonora de sistema sem imposição de controladores de emulação de terceiros é executada através da interface WASAPI (Windows Audio Session API) em modo *Loopback*. Este subsistema permite à aplicação intercetar diretamente o sinal de áudio digital direcionado para o dispositivo de reprodução principal através da inicialização de uma instância de `IAudioClient` com a flag de ativação `AUDCLNT_STREAMFLAGS_LOOPBACK`.

Para prevenir desfasamentos temporais e degradação provocada por interrupções do escalonador do Windows, a linha de execução (*thread*) responsável pela recolha contínua dos amortecedores PCM inscreve-se no serviço MMCSS (Multimedia Class Scheduler Service) mediante o registo sob o perfil "Pro Audio". Desta forma, o sistema operativo reserva recursos dedicados do processador para a rotina de áudio, mantendo os tempos de amortecimento em níveis inferiores a $5\text{ ms}$. As amostras de ponto flutuante de 32 bits a 48 kHz são posteriormente encaminhadas para um codificador Opus em tempo real, empacotadas em blocos sonoros com durações de $5\text{ a }10\text{ ms}$ a $128\text{ kbps}$, assegurando sincronização temporal constante face aos carimbos temporais de apresentação (PTS) do fluxo de vídeo.

---

## 4. Topologia de Rede e Protocolo de Transporte sob Radmin VPN

A integração da infraestrutura de rede na plataforma Radmin VPN baseia-se na criação de um túnel encriptado de Camada 2/3 que emula uma rede local de computadores entre nós fisicamente distantes através da internet. Todavia, ao contrário de redes locais físicas por cabo de par entrançado (onde colisões e congestionamentos são marginais), o túnel virtual da Radmin VPN está sujeito a variações de rota na Internet pública, o que exige um protocolo de transporte de baixa sobrecarga.

### 4.1 Comportamento da Camada L2/L3 e Controlo Rigoroso de MTU

O recurso ao protocolo TCP é terminantemente inviável para vídeo em tempo real a 120 FPS. As rotinas de retransmissão cumulativa do TCP e os algoritmos de prevenção de congestionamento provocam o bloqueio de cabeça de linha (*head-of-line blocking*): a perda de um único segmento de rede interrompe a entrega de todos os segmentos subsequentes até que ocorra a retransmissão com sucesso, traduzindo-se em congelamentos visuais súbitos e acumulação exponencial de latência.

Assim, o transporte de dados do sistema baseia-se exclusivamente no protocolo UDP, recorrendo a uma implementação otimizada do protocolo RTP (Real-time Transport Protocol). Uma restrição primordial imposta pela Radmin VPN decorre da dimensão do pacote de dados.

A interface física de rede opera habitualmente com uma Unidade Máxima de Transmissão de 1500 bytes. Quando o adaptador virtual da Radmin VPN encapsula o tráfego em pacotes criptografados adicionais, consome espaço nos cabeçalhos. Caso a aplicação transmita pacotes UDP superiores ao MTU suportado pelo túnel, o sistema operativo executa a fragmentação de pacotes ao nível do protocolo IP. A fragmentação IP degrada substancialmente o desempenho: a perda de um único fragmento no percurso inviabiliza a remontagem de todo o datagrama UDP pelo sistema operativo recetor, amplificando as perdas de rede.

Por conseguinte, a camada de empacotamento da aplicação deve impor um limite rigoroso à dimensão máxima da carga útil dos seus pacotes:

$$\text{Tamanho Máximo do Pacote (Payload UDP + RTP)} \le 1400\text{ bytes}$$

As Unidades de Camada de Abstração de Rede (NAL units) geradas pelo codificador de vídeo — cujas dimensões num fotograma 4K podem ultrapassar $50\text{ kB}$ — são fragmentadas em múltiplos subpacotes através do formato de empacotamento de unidades de fragmentação (compatível com as especificações RFC 7798 para HEVC), garantindo que nenhum datagrama exceda o patamar estipulado de 1400 bytes.

### 4.2 Mecanismos de Transporte e Correção de Erros (Reed-Solomon FEC)

A cadência temporal restrita de $8,33\text{ ms}$ a 120 FPS não concede margem temporal suficiente para que o cliente recetor reporte perdas de pacotes e aguarde pela respetiva retransmissão via sinalização NACK através do túnel da VPN. O tempo de viagem de ida e volta (*Round-Trip Time* - RTT) da ligação virtual raramente se situa abaixo dos $10\text{ a }20\text{ ms}$, tornando qualquer retransmissão reativa obsoleta face ao fotograma já em processamento na linha temporal de apresentação.

Para resolver esta limitação, a arquitetura introduz um mecanismo de Correção de Erros para a Frente (*Forward Error Correction* - FEC) baseado em códigos de bloco de Reed-Solomon. O funcionamento deste pipeline de proteção e transporte de rede organiza-se da seguinte forma:

1. O fotograma comprimido proveniente do codificador é segmentado em $k$ pacotes de dados de tamanho regular.
2. De seguida, o codificador Reed-Solomon aplica matrizes de transformação no corpo de Galois $GF(2^8)$ para sintetizar $m$ pacotes adicionais de paridade matemática, estabelecendo uma redundância dinâmica pré-configurada entre 10% e 20%.
3. Os pacotes de dados e de paridade recebem cabeçalhos RTP normalizados, contendo números de sequência sequenciais, carimbos temporais monotónicos e identificadores de sincronização de fonte (SSRC).
4. Finalmente, os pacotes são remetidos através de sockets UDP sem bloqueio diretamente para o endereço IP do adaptador Radmin VPN do destinatário.

No terminal recetor, a receção de quaisquer $k$ pacotes pertencentes ao agrupamento de $k + m$ pacotes permite à álgebra matricial de Reed-Solomon reconstruir integralmente os pacotes em falta de forma instantânea na memória do cliente, neutralizando quebras na transmissão sem requisições de retransmissão.

**Tabela: Comparativo de Protocolos para Partilha de Ecrã 4K120**

| Protocolo / Mecanismo | Comportamento a 120 FPS | Sobrecarga de Pilha | Resiliência a Perdas | Adequação Arquitetural |
| :--- | :--- | :--- | :--- | :--- |
| **TCP Sockets Padrão** | Falha estrutural por acumulação de fila | Mínima | Retransmissão obrigatória (induz paragens) | Inadmissível |
| **WebRTC Completo (Google M98+)** | Moderada com afinação complexa | Muito Elevada | Retransmissão NACK combinada com FEC | Excessivamente pesado para UI minimalista |
| **Pilha libdatachannel** | Elevada eficácia e controlo | Baixa/Média | Protocolo SCTP/SRTP nativo em C++ | Elevada aptidão para canais P2P |
| **RTP Customizado sobre UDP + FEC** | Desempenho ótimo e determinístico | Mínima | Reed-Solomon FEC direto por fotograma | Excelente (Alinhamento direto com hardware) |

---

## 5. O Plano Diretor de Engenharia em Quatro Etapas Arquiteturais

A estrutura global da solução está dividida em quatro etapas conceptuais bem delimitadas. A transição entre cada etapa depende da estabilização e validação das interfaces funcionais precedentes, assegurando que o sistema não incorra em débitos técnicos ocultos.

### Etapa 1: Núcleo de Captura Zero-Copy e Temporização Gráfica
Esta fase foca-se na abstração do subsistema de vídeo do Windows para obtenção limpa e de baixa latência da imagem do monitor. Compreende a inicialização do dispositivo Direct3D 11 (`ID3D11Device`) com suporte de aceleração por hardware e criação das rotinas de captura através da API Desktop Duplication (`IDXGIOutputDuplication`). Concomitantemente, é integrado o canal alternativo via `Windows.Graphics.Capture` para mitigação de incompatibilidades com composições de janelas MPO.
A etapa é finalizada com a construção de um ciclo de interrogação (*polling loop*) de alto rendimento ancorado no temporizador de alta precisão QPC, capaz de entregar a textura gráfica em memória VRAM respeitando a cadência de 120 FPS ($8,333\text{ ms}$) sem retenção de fotogramas redundantes.

### Etapa 2: Aceleração Gráfica de Vídeo e Ingestão de Áudio
Esta fase materializa o pipeline de processamento e codificação em tempo real diretamente na placa gráfica. O componente inicializa uma sessão NVENC através do SDK da NVIDIA (ou o respetivo SDK AMF em placas AMD), ligando diretamente a textura D3D11 obtida na Etapa 1 sem qualquer transferência de dados pelo barramento do sistema. Ocorre a conversão matemática de cor através de um sombreador interno para o formato NV12.
Em simultâneo, estabelece-se o subsistema áudio baseado na interface WASAPI Loopback, capturando o fluxo de som do sistema operacional em modo partilhado de baixa latência e despachando os blocos sonoros para compressão em pacotes Opus a 48 kHz sincronizados com os carimbos temporais de vídeo.

### Etapa 3: Motor de Transporte de Rede Ponto-a-Ponto e Resiliência a Perdas
Nesta fase, constrói-se o sistema de comunicação de rede ponto-a-ponto dimensionado para o túnel da Radmin VPN. O motor consome os blocos de dados de vídeo codificados (NAL units), dividindo-os em pacotes com dimensão estritamente confinada ao teto de 1400 bytes, de modo a prevenir a fragmentação IP pela placa de rede virtual.
Implementa-se o algoritmo matricial de correção Reed-Solomon para gerar pacotes de paridade FEC e associam-se os cabeçalhos normativos de transporte RTP. Os pacotes resultantes são emitidos e recebidos através de sockets UDP não bloqueantes em conformidade com as primitivas da API Winsock do Windows, garantindo que oscilações transitórias na ligação virtualizada não causem congelamento de fotogramas.

### Etapa 4: Interface de Utilizador (UI), Descodificação por Hardware e Apresentação
A fase final integra os módulos de software numa aplicação funcional orientada ao utilizador. Constrói-se uma interface gráfica contemporânea com estética limpa inspirada no Discord, recorrendo a uma biblioteca de visualização de alto desempenho (como Dear ImGui acelerado sobre Direct3D 11 ou Qt 6 / QML sem bufferização profunda). A interface incorpora seletores de resolução (4K, 1440p, 1080p), cadência (60 FPS, 120 FPS), campo de introdução do endereço IP da Radmin VPN e métricas de diagnóstico em tempo real.
O componente cliente integra a receção dos fluxos de dados, a reconstrução FEC e a invocação da descodificação acelerada por hardware via Direct3D 11 Video Acceleration (D3D11VA), culminando na renderização dos fotogramas numa cadeia de permuta gráfica (*Swapchain*) configurada em modo `DXGI_SWAP_EFFECT_FLIP_DISCARD` com V-Sync desativado para apresentação imediata.

---

## 6. Especificação Operacional em Quatro Lotes de Execução para Agentes de IA

Para assegurar que ambientes automatizados de engenharia de software baseados em modelos de linguagem de grande escala (LLMs como Claude Code, Codex ou Antigravity) processem a construção do sistema sem sofrer de degradação de contexto ou dispersão de requisitos, o projeto é estruturado em quatro lotes atómicos de implementação. Cada lote define ficheiros específicos, interfaces formais em C++20 e comandos determinísticos de teste e validação que o agente deve executar para confirmar a conclusão de cada etapa antes de avançar para a subsequente.

A pilha de ferramentas selecionada assenta no compilador Microsoft Visual C++ (MSVC 2022 ou ferramentas de compilação C++ de linha de comando), gestão de configuração através do CMake (versão 3.25 ou superior) e controlo de dependências externas via `vcpkg` em modo de manifesto.

**Tabela: Especificação dos Lotes de Execução**

| Lote de Execução | Componentes Desenvolvidos | Dependências de Sistema | Ficheiros de Origem | Validação Determinística |
| :--- | :--- | :--- | :--- | :--- |
| **Lote 1** | Captura D3D11 (DXGI / WGC) | Windows SDK, Direct3D 11 | `src/capture/*`, `tests/test_capture.cpp` | `ctest -R test_capture` (120 FPS sem perda) |
| **Lote 2** | Codificação NVENC e Áudio | NVIDIA Codec SDK, libopus | `src/encoder/*`, `src/audio/*` | `ctest -R test_encoder` (latência $\le 3\text{ ms}$) |
| **Lote 3** | Rede UDP/RTP e Reed-Solomon | Winsock2, Galois Field Lib | `src/network/*`, `tests/test_network.cpp` | `ctest -R test_network` (recupera 5% perda) |
| **Lote 4** | Interface Gráfica e Apresentação | ImGui / Qt6, D3D11 Swapchain | `src/ui/*`, `src/client/*`, `src/main.cpp` | Execução integrada ponto-a-ponto via Radmin |

### Lote 1: Scaffolding do Projeto, Pipeline de Captura D3D11 e Temporização

O primeiro lote estabelece o esqueleto do repositório, o ficheiro mestre de compilação CMake, a configuração de dependências do vcpkg e a implementação do subsistema de captura direta de ecrã em Direct3D 11. O agente deve assegurar que a textura gerada resida estritamente em VRAM.

**Directiva de Execução para o Agente de IA:**
Implementar o Lote 1 da aplicação de partilha de ecrã (ScreenShare4K).
Ambiente: C++20, compilador MSVC x64, CMake 3.25+.

**Estrutura de ficheiros a criar:**
- `CMakeLists.txt` (Definição de projeto, flags `/O2 /Oi /GL /std:c++20`, linking com `d3d11.lib`, `dxgi.lib`, `windowsapp.lib`)
- `vcpkg.json` (Manifesto de dependências)
- `src/capture/ICaptureSource.h` (Interface abstrata de captura)
- `src/capture/CaptureEngine.h` e `src/capture/CaptureEngine.cpp` (Fachada com fallback automático)
- `src/capture/DXGICapture.h` e `src/capture/DXGICapture.cpp` (Captura via IDXGIOutputDuplication)
- `src/capture/WGCCapture.h` e `src/capture/WGCCapture.cpp` (Captura via Windows.Graphics.Capture WinRT)
- `tests/test_capture.cpp` (Teste unitário e de desempenho)

**Requisitos de Código e Interfaces:**
1. Em `ICaptureSource.h`, estabelecer:
```cpp
virtual bool Initialize(int displayIndex, int targetFps) = 0;
virtual bool AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) = 0;
virtual void ReleaseFrame() = 0;
```
2. Em `DXGICapture`, criar o `ID3D11Device` com a flag `D3D11_CREATE_DEVICE_VIDEO_SUPPORT`.
3. Consumir fotogramas com `AcquireNextFrame(0, &FrameInfo, &pResource)`. Em caso de `DXGI_ERROR_WAIT_TIMEOUT`, não falhar: reaproveitar a textura anterior marcando o fotograma como duplicado sem cópia de memória.
4. Em `WGCCapture`, instanciar `Direct3D11CaptureFramePool::CreateFreeThreaded` com formato `DirectXPixelFormat::B8G8R8A8UIntNormalized`. Definir `IsBorderRequired = false` para eliminar a moldura amarela do Windows.
5. Em `tests/test_capture.cpp`, executar um ciclo de captura a 3840x2160 a 120 FPS durante 10 segundos contínuos (1200 fotogramas teóricos). Registar a média de tempo de processamento por fotograma através de `QueryPerformanceCounter`.

**Critério de Aceitação:**
Compilação sem erros no MSVC e passagem com sucesso no teste `test_capture.exe` com registo de pelo menos 118 FPS efetivos e sem transi

### Lote 2: Codificador NVENC em Memória Gráfica e Captura de Áudio WASAPI

O segundo lote conecta a saída visual em VRAM do Lote 1 ao codificador de hardware da GPU e estrutura o módulo de captura do áudio do sistema com codificação Opus simultânea.

**Directiva de Execução para o Agente de IA:**
Implementar o Lote 2 da aplicação ScreenShare4K integrado com o Lote 1.
Dependências: NVIDIA Video Codec SDK (headers `nvEncodeAPI.h`) e `libopus` (adicionar ao `vcpkg.json`).

**Estrutura de ficheiros a criar:**
- `src/encoder/VideoEncoderNVENC.h` e `src/encoder/VideoEncoderNVENC.cpp`
- `src/encoder/ColorConverterD3D11.h` e `src/encoder/ColorConverterD3D11.cpp` (Shader BGRA para NV12)
- `src/audio/WasapiLoopback.h` e `src/audio/WasapiLoopback.cpp`
- `src/audio/OpusAudioEncoder.h` e `src/audio/OpusAudioEncoder.cpp`
- `tests/test_encoder.cpp`

**Requisitos de Código e Interfaces:**
1. Em `VideoEncoderNVENC`, carregar dinamicamente `nvEncodeAPI64.dll`. Abrir a sessão de codificação sobre o `ID3D11Device` instanciado no Lote 1.
2. Configurar o perfil para HEVC (`NV_ENC_CODEC_HEVC_GUID`) com afinação `NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY` e predefinição `NV_ENC_PRESET_P1_GUID`.
3. Desativar B-Frames (`frameIntervalP = 1`) e definir modo de débito CBR a 85.000 kbps com `enablePTD = 1`.
4. Mapear a textura Direct3D 11 convertida em NV12 utilizando `nvEncRegisterResource` com `NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX`, assegurando arquitetura Zero-Copy.
5. Em `WasapiLoopback`, inicializar o `IAudioClient` em modo `AUDCLNT_STREAMFLAGS_LOOPBACK` associado à categoria MMCSS "Pro Audio". Extrair as amostras PCM a 48 kHz (estéreo, float 32-bit).
6. Em `OpusAudioEncoder`, inicializar a biblioteca Opus com amostragem para pacotes de 10 ms (480 amostras por canal) a 128 kbps.
7. Em `tests/test_encoder.cpp`, alimentar a cadeia de captura do Lote 1 diretamente para o codificador de vídeo e processar o áudio de sistema durante 5 segundos.

**Critério de Aceitação:**
Geração de fluxo binário válido H.265 em tempo real com tempo de serviço por fotograma inferior a 2,5 ms no NVENC e emissão síncrona de áudio.

### Lote 3: Protocolo de Transporte UDP/RTP, Fragmentação de Pacotes e Reed-Solomon FEC

O terceiro lote estabelece a infraestrutura de telecomunicações ponto-a-ponto para difusão sobre o túnel virtual da Radmin VPN, garantindo a integridade dos dados face a eventuais perdas de rede.

**Directiva de Execução para o Agente de IA:**
Implementar o Lote 3 da aplicação ScreenShare4K (Camada de Rede e Resiliência).
Contexto: A comunicação opera sob a interface virtual da Radmin VPN (gama de IPs 26.x.x.x, MTU restrito).

**Estrutura de ficheiros a criar:**
- `src/network/RtpHeader.h` (Estrutura binária do cabeçalho RTP com alinhamento de 1 byte)
- `src/network/RtpPacketizer.h` e `src/network/RtpPacketizer.cpp`
- `src/network/ReedSolomonFec.h` e `src/network/ReedSolomonFec.cpp` (Cálculo em corpo de Galois GF(2^8))
- `src/network/UdpTransport.h` e `src/network/UdpTransport.cpp` (Comunicação assíncrona Winsock)
- `tests/test_network.cpp`

**Requisitos de Código e Interfaces:**
1. Em `RtpPacketizer`, segmentar unidades NAL de vídeo maiores do que 1360 bytes em múltiplos fragmentos com cabeçalhos de fragmentação normalizados (deixando margem para cabeçalhos RTP/UDP/IP dentro do MTU de 1400 bytes da Radmin VPN).
2. Em `ReedSolomonFec`, receber matrizes de N pacotes de dados e produzir M pacotes de redundância de paridade (configuração base: 10 pacotes de dados para 2 pacotes de paridade, tolerando 20% de perda).
3. Em `UdpTransport`, configurar sockets Winsock em modo não-bloqueante (`ioctlsocket FIONBIO`). Expandir os amortecedores do sistema operacional para 8 MB (`SO_SNDBUF` e `SO_RCVBUF`) de modo a suportar rajadas de tráfego de 4K sem descarte ao nível do kernel.
4. Disponibilizar suporte de ligação ao endereço IPv4 local do adaptador Radmin VPN e envio direto para o IP de destino pretendido.
5. Em `tests/test_network.cpp`, gerar um fluxo sintético equivalente a 85 Mbps de tráfego RTP, introduzir artificialmente 5% de perda aleatória de pacotes num canal emulado e validar a recuperação integral dos dados originais pelo descodificador FEC.

**Critério de Aceitação:**
Passagem sem falhas no teste `test_network.exe` com reconstituição integral dos fotogramas corrompidos e sem qualquer chamada bloqueante.

### Lote 4: Descodificador D3D11VA, Renderização Flip-Model e Interface Gráfica

O quarto lote implementa o nó cliente recetor, unifica a cadeia de processamento com a descodificação por hardware e desenha a interface gráfica do utilizador para controlo da aplicação.

**Directiva de Execução para o Agente de IA:**
Implementar o Lote 4 da aplicação ScreenShare4K: Interface de Utilizador e Motor de Apresentação Cliente.
Interface: C++ recorrendo a ImGui renderizado sobre DirectX 11 (ou Qt 6 sem bufferização interna profunda).

**Estrutura de ficheiros a criar:**
- `src/client/D3D11VaDecoder.h` e `src/client/D3D11VaDecoder.cpp` (Descodificação por hardware)
- `src/client/PresentationEngine.h` e `src/client/PresentationEngine.cpp` (Swapchain D3D11 Flip-Model)
- `src/ui/AppWindow.h` e `src/ui/AppWindow.cpp` (Interface gráfica ao estilo Discord)
- `src/main.cpp` (Ponto de entrada integrado da aplicação)

**Requisitos de Código e Interfaces:**
1. Em `D3D11VaDecoder`, receber os pacotes RTP remontados, reconstruir as unidades NAL e submeter os dados para descodificação direta por hardware através da API Direct3D 11 Video (ou FFmpeg libavcodec com hwaccel configurado estritamente para d3d11va). Despejar a superfície descodificada diretamente para uma textura `ID3D11Texture2D`.
2. Em `PresentationEngine`, configurar a `IDXGISwapChain1` com a propriedade `DXGI_SWAP_EFFECT_FLIP_DISCARD` e associar `DXGI_SCALING_STRETCH`. Executar `Present(0, 0)` para renderização imediata desacoplada do intervalo de varrimento vertical (V-Sync desativado).
3. Em `AppWindow`, desenhar uma interface contemporânea minimalista inspirada no Discord:
   - Modos alternáveis: "Transmitir Ecrã" (Hospedeiro) ou "Assistir Transmissão" (Recetor).
   - Menus seletores de Resolução: 4K (3840x2160), 1440p (2560x1440) e 1080p (1920x1080).
   - Menus seletores de Framerate: 60 FPS e 120 FPS.
   - Campo de entrada de texto com validação para o endereço IP do parceiro na Radmin VPN (formato 26.x.x.x).
   - Painel de telemetria em tempo real: Latência Fim-a-Fim (ms), Débito de Rede Efetivo (Mbps), FPS de Renderização e Rácio de Perda de Pacotes.
4. Em `src/main.cpp`, unir os módulos dos Lotes 1, 2, 3 e 4 num único binário executável (`ScreenShareApp.exe`).

**Critério de Aceitação:**
Execução estável de uma sessão interativa de partilha de ecrã a 4K 120 FPS através da interface virtual da Radmin VPN, validação do swapchain sem tearing severo.
