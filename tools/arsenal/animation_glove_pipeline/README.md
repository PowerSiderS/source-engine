# Animações fornecidas e luvas CSSO

Usa os modelos locais de `cs2 pack v1/cs2 test pack` (ports Source 1, não arquivos Source 2) e as luvas de `csso_release_1.1/csso`. Os assets ficam na pasta de trabalho, fora do Git. A procedência local não comprova autenticidade Valve nem concede licença para distribuir os assets.

Defina `SA_ANIMATION_WORK` para usar outra pasta com os intermediários e as ferramentas. O padrão é a pasta de trabalho desta atualização. Dependências: Crowbar compilado localmente, `studiomdl.exe` do SDK 2013 Multiplayer, NumPy e o VPK local. `prepare_csso_gloves.py` também usa `extract_native_knives.py` e `inspect_source_mdl.py` da pasta de trabalho anterior.

Ordem: `decompile_cs2_pack.py`, `build_cs2_pack.py`, `prepare_csso_gloves.py`, `build_csso_gloves.py`, `audit_wrist_binding.py`, `stage_animation_gloves.py`, `package_animation_gloves.py`, `test_animation_gloves.py`.

São 24 modelos de armas de fogo: 10 usam o rig `arm_lower` e 14 conservam `Bip01/ForeTwist`. As sequências numéricas originais são preservadas; apenas nomes de bones, caminhos de materiais e eventos reconhecidos pela engine são adaptados. Os modelos de mundo, gameplay e 21 facas existentes são conservados.

São 20 estilos de luvas CSSO, cada um compilado para três rigs. A conversão CS2 usa frames anatômicos de palma e antebraço, ancora a malha no pulso e associa o antebraço a `arm_lower_[LR]_TWIST1`. Isso evita fundir rotações incompatíveis no punho e respeita a inversão dos eixos do braço direito. UVs/topologia são preservadas. O shader CSSO `Character`, ausente nesta engine, usa `VertexLitGeneric` com as texturas originais: iluminação não é idêntica à do CSSO.

Para iterar uma luva: `build_csso_gloves.py --rig=cs2 --style=v_glove_sporty`. `--materials-only` atualiza apenas dependências de materiais. Refaça a compilação completa antes de publicar.

O VPK oficial é gerado em chunks com `-M -c 100`, evitando o limite de memória do utilitário x86, e convertido por streaming para VPK v2 único. CRC32 e payload são conferidos contra cada arquivo de staging. `skins_manifest.txt` fica solto e editável, fora do VPK.

`test_animation_gloves.py --wrist` captura repouso/disparo/recarga/inspeção no jogo privado. O teste completo verifica 24 armas, 60 combinações de luva/rig, troca no meio da partida, mudanças de vídeo e Nuke. O teste só limpa entidades e altera binds/configuração na instância privada. Usa screenshot TGA, não JPEG. Matriz finita não prova qualidade visual: revise as capturas também.

Build normal usa `WAFLOCK=.lock-waf-native-knife-build`; dedicated usa `.lock-waf-native-dedicated-build`. Ambos estão configurados nos diretórios `build`/`build-dedicated`. Execute `python waf build install -j8 --targets=client,server,GameUI` no normal e `--targets=server` no dedicated. Não substitua a configuração Waf de testes em uso por outro editor.

A categoria **Gloves** é lida do manifesto no Inventário. `inventory_gloves 4000..4019` aplica imediatamente. `inventory_gloves 0` restaura o padrão CSSO Sporty (4019). A malha de braço embutida no pacote de armas fica escondida após carregar a luva CSSO compatível e serve apenas de fallback se o carregamento falhar.

Esta atualização de cosméticos não conclui a medição de carregamento dos dez mapas nem a opção de proporção esticada; esses trabalhos têm validação própria.
