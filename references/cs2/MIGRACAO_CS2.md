# Migração CS2 / apresentação CS:GO

Estado da atualização em preparação. Ainda não instalada no runtime principal. A instalação ocorre após validação das DLLs e dos assets juntos.

| Item | Sistema | Estado | Evidência / limite |
|---|---|---|---|
| 01 | Inventário dos arquivos locais do CS2 | Concluído | VPK lido com CRC; snapshot de 2026-10-01. |
| 02 | Ferramenta de extração Source 2 Viewer | Concluído | CLI 20.0 do projeto original; SHA-256 da release verificado. |
| 03 | Hitboxes de jogadores | Em teste | 19 cápsulas × 8 rigs; radii/endpoints CS2 adaptados às poses Source 1; 152 geometrias auditadas. |
| 04 | Bones e alinhamento animado | Em teste | Esqueleto e animações Source 1 preservados; ainda precisa teste em pé/agachado/rotação/disparo. |
| 05 | Recoil e spray por arma | Em teste | 34 perfis atuais compilados; sementes/amplitudes/variâncias extraídas. Gerador determinístico Source 1 permanece; equivalência Source 2 não comprovada. |
| 06 | Precisão e recuperação | Em teste | Valores por modo, postura, movimento, salto e transição de recuperação copiados para os 34 perfis. |
| 07 | Economia e preços | Em teste | 38 entradas, bônus de perda progressivo, recompensas de arma/objetivo, MR12 e munição gratuita; falta validação de transações reais. |
| 08 | HUD clássico CS:GO rosa | Em teste | Layout VGUI: radar/dinheiro, relógio/placar/vivos, vida/colete e munição; atlas de ícones limpos. |
| 09 | Menu e inventário estilo CS:GO | Em implementação | Grade e categorias locais; acesso direto em janela ampla. Não importa serviços Steam/Panorama. |
| 10 | Animações de armas | Em teste | 24 modelos Source 1 do pacote fornecido, motion original preservado. Não são uma extração direta/autenticada dos modelos Source 2. |
| 11 | Luvas CSSO e pulsos | Em teste | 20 estilos × 3 rigs; quatro malhas sport.smd antes não ocultadas foram corrigidas. Revisão visual restante. |
| 12 | Áudio CS2 | Em implementação | Conversão de disparos locais para PCM compatível; mixagem espacial/acústica Source 1 continua. |
| 13 | Dano, headshot e penetração | Parcial | Dano base, alcance e armor ratio nos perfis; multiplicador por arma, penetração float/materiais ainda exigem integração. |
| 14 | Granadas e utilitários | Pendente | Arremessos já existentes; revisar com medições gravidade/quiques/pavio/HE/flash/fogo. Não copiar algoritmos desconhecidos dos assets. |
| 15 | Movimentação e bunny hop | Pendente | Velocidade por arma atualizada; aceleração/fadiga/counter-strafe/ladders precisam validação separada. |
| 16 | Modelos Source 2 completos | Requer conversão | VMDL/VMESH/material Source 2 não carrega como MDL/VTX/VVD. Exportar e retargetar malha/material/animação exige pipeline próprio. |
| 17 | Smoke volumétrica CS2 | Requer implementação própria | Partículas e assets não fornecem o código de voxels, sincronização e interação com balas. |
| 18 | Subtick e networking CS2 | Requer implementação própria | Esta engine mantém ticks Source 1; nenhum asset converte seu protocolo e predição para Source 2. |
| 19 | Carregamentos e resolução | Parcial | Correções de engine existentes; relatório de tempo real dos dez mapas e conexão dedicada ainda incompleto. |
| 20 | Mapas do usuário e dedicated | Em teste | Usar somente catálogo local permitido; VPK/DLLs comuns e auditoria nativa no servidor. |
| 21 | Proporção de tela esticada | Pendente | Separar proporção de renderização da resolução de saída; pedido anterior ainda não concluído. |
| 22 | Anti-cheat | Parcial | Sistema existente; não pode importar VAC/serviços privados da Valve como assets. Revisão futura contra falsos positivos. |

Os arquivos locais servem como referência de dados. Não contêm a implementação completa da Source 2, de subtick ou de VAC. CS:GO 2015, CS:GO Legacy e CS2 são versões diferentes; este pacote usa dados atuais do CS2 e apresentação inspirada no CS:GO.

Extração: [Source 2 Viewer / ValveResourceFormat](https://github.com/ValveResourceFormat/ValveResourceFormat). As malhas/animações de armas do pacote fornecido e as luvas CSSO têm proveniência separada dos dados oficiais do CS2.
