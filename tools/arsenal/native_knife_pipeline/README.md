# Pipeline de facas nativas

Os scripts reutilizam assets locais e nunca executam instaladores de terceiros. A extração verifica CRC32; a recompilação mantém os quadros numéricos dos SMDs. Fontes: pacote CSSO local e malha de luvas da Glock existente. Isso não autentica os assets como arquivos Valve de 2015.

Sequência: extract_native_knives.py → decompile_native_knives.py → build_native_knife_assets.py → package_native_catalog.py. Requer Crowbar local compilado, studiomdl do SDK 2013 e VPK. Os caminhos da instalação/source são os do projeto deste computador. SA_KNIFE_WORK permite escolher outra pasta de trabalho com essas dependências e o catálogo anterior. Sem essa variável, os scripts usam a pasta de trabalho desta atualização, preservando todos os SMD/QC intermediários.

Build do cliente: python waf configure -T release --build-games=cstrike --enable-opus --disable-warns --prefix=output --out=build; depois python waf -o build build install -j 8. Dedicated usa --dedicated, --prefix=output-dedicated e --out=build-dedicated. Esses comandos geram DLLs; não alteram automaticamente a instalação principal.

setup_native_runtime.py --candidate prepara uma instância privada que usa conteúdos compartilhados da instalação, com DLLs e modelos próprios. Coloque o VPK gerado no custom dessa instância. test_native_knives.py valida as 21 inspeções, ataques, bones e mudanças reais de resolução. unbindall/sensitivity 0 são aplicados apenas na configuração temporária privada, para evitar entrada manual no teste; a configuração da instalação não é alterada.

A engine registra todo o catálogo sem força de preload. Primeira seleção ainda pode carregar o modelo; seleção durante a partida permanece disponível. mat_fast_video_changes usa ResetEx com preservação de texturas em D3D9Ex e fallback integral em dispositivos antigos.

Teste de todos os mapas: test_native_maps.py. Cria outro runtime privado, carrega os dez mapas do catálogo, exercita três rigs de faca e a troca para Glock, efetua quatro modos reais de vídeo em cada mapa e encerra normalmente. Os resultados ficam em all-maps-test/result.json.

O teste também chama staticprop_validate_handles em cada mapa. Essa auditoria compara a identificação e o lookup de todos os índices reais, incluindo o intervalo acima de 4.095 presente no Nuke. --map de_nuke_csgo_new executa apenas o teste isolado e salva resultado em outra pasta, sem substituir a validação completa.

Após a validação, profile_map_loading.py faz uma rodada independente de changelevel nos dez mapas, medindo servidor pronto e cliente active/SIGNON_FULL. O resultado fica em map-loading-profile/result.json. Usa cache aquecido pelos testes anteriores, sem bots, janela 800x600; não equivale a benchmark com cache frio. As portas e configurações são privadas.

O término da medição exige o marcador client_ready de cl_profile_map_load, emitido depois de finalizar CL_FullyConnected e fechar a tela de carregamento. A indicação active no servidor, isolada, não é usada como conclusão. O diagnóstico fica desligado por padrão. profile_dedicated_connections.py repete a medição com dedicated e cliente separados, também privados e em loopback, para distinguir carregamento de servidor e cliente sem latência de Internet.
