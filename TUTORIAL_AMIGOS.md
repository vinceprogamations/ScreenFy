# 🚀 Guia Rápido: Como Usar o ScreenFy com os seus Amigos

O **ScreenFy** permite partilhar o seu ecrã em **4K a 120 FPS / 60 FPS** com som do sistema e ultra-baixa latência (< 15ms) através de uma conexão direta ponto-a-ponto (P2P).

---

## 📋 O que os seus amigos precisam de ter:

1. **Radmin VPN** (gratuito e sem limite de velocidade): [https://www.radmin-vpn.com/](https://www.radmin-vpn.com/)
2. Os ficheiros do ScreenFy:
   - `ScreenShare.exe`
   - `opus.dll` (deve estar na mesma pasta que o executável)

---

## 🛠️ Passo a Passo Simples (3 Minutos)

### Passo 1: Ligar à mesma rede no Radmin VPN
1. Abra o **Radmin VPN**.
2. Um de vocês clica em **Rede** ➔ **Criar Nova Rede** (escolha um nome e senha, por exemplo: `Galera4K`).
3. Os outros amigos clicam em **Rede** ➔ **Entrar numa Rede Existente** e colocam o mesmo nome e senha.
4. Pronto! O vosso IP do Radmin VPN será algo como `26.x.x.x`.

---

### Passo 2: Abrir o ScreenFy
1. Dê duplo clique em **`ScreenShare.exe`**.
2. O ScreenFy detetará automaticamente o seu IP da Radmin VPN e abrirá a interface estilo Discord.

---

### Passo 3: Adicionar o seu Amigo
1. No ecrã inicial do ScreenFy, clique no botão roxo **"Copiar Código"** (este é o seu *Friend Code*).
2. Envie esse código ao seu amigo (pelo Discord, WhatsApp, etc.).
3. O seu amigo abre o ScreenFy dele, clica no botão verde **"+ Adicionar Amigo"** na barra lateral esquerda e cola o seu código.
4. Vocês aparecerão automaticamente na lista de contactos um do outro com o indicador **ONLINE 🟢**!

---

### Passo 4: Testar o Ecrã (Preview ao Vivo)
1. Antes de ligar, clique no botão **"👁 Testar Ecrã (Preview)"**.
2. Aparecerá uma miniatura em tempo real do seu ecrã na tela com a indicação de FPS, confirmando que o monitor certo está a ser capturado!
3. Pode parar o teste a qualquer momento no botão **"⏹ Parar Teste"**.

---

### Passo 5: Iniciar a Transmissão em 4K
1. Clique no nome do seu amigo na lista à esquerda.
2. Escolha a qualidade desejada:
   - **4K (3840x2160)** ou **1080p**
   - **120 FPS** (ultra fluido para jogos) ou **60 FPS**
3. Clique no botão grande verde **"▶ INICIAR TRANSMISSÃO DE ECRÃ"**.
4. No ecrã do seu amigo, surgirá um aviso pop-up com som. Ele clica em **"Aceitar"**.
5. **A transmissão em 4K com áudio começa instantaneamente!**

---

---

## 🛠️ Para Amigos que baixarem o projeto do GitHub (Compilação em 1 Clique)

1. Baixe ou clone o repositório do GitHub.
2. Execute **`install_tools.bat`** (clique direito ➔ Executar como Administrador).
   - O instalador detecta automaticamente seu Visual Studio, instala CMake e vcpkg.
3. Dê duplo clique em **`build.bat`** (ou execute o `build_gui.ps1` no PowerShell).
   - O projeto compilará em modo Release e gerará o `ScreenShare.exe` pronto na raiz!
4. Se precisar limpar arquivos no futuro, use **`uninstall_tools.bat`** (ele tem menu interativo e nunca apaga o seu código).

---

## 👤 Personalização: Alterar Nome, Foto e Status

1. Clique na engrenagem **`⚙`** no rodapé (ou clique no seu perfil).
2. Na aba **"Minha Conta"**:
   - **Foto de Perfil**: Clique em **"📷 Alterar Foto"** e escolha qualquer imagem PNG, JPG ou BMP.
   - **Nome**: Digite o novo nome e clique em **"Guardar Nome"**.
   - **Estado**: Alterne entre 🟢 Online, 🟡 Ausente, 🔴 Não Incomodar ou ⚪ Invisível.
3. Seus amigos verão seu avatar e status atualizados instantaneamente!

---

## 🔍 Algo deu errado? Como usar os Logs Inteligentes:

Se houver qualquer problema de conexão, som ou captura:
1. Abra as Configurações **`⚙`** ➔ Aba **"Diagnóstico & Registos"**.
2. Você verá todos os logs dos motores gráficos e de rede em tempo real.
3. Clique em **"📋 Copiar Diagnóstico Completo"** e envie para seu amigo no Discord ou GitHub!
4. O diagnóstico inclui detalhes da sua GPU, resoluções, portas UDP e mensagens de erro explicativas.

---

## ⚙️ Dicas e Atalhos Úteis:

- **Fechar Janelas / Menus**: Carregue na tecla **ESC** para fechar definições instantaneamente.
- **Configurações Rápidas**: No rodapé à esquerda, clique na engrenagem **`⚙`** para escolher qual o monitor quer partilhar, volume do microfone ou bitrate de rede.
- **Ajuda Integrada**: No rodapé, clique em **`?`** para ver este guia diretamente dentro da aplicação e copiá-lo com 1 clique!
- **Modo em Transmissão**: Ao passar o rato na parte inferior do ecrã, surge uma barra flutuante para **Mutar Microfone** ou **Desligar a Chamada**.
