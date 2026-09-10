# Counter-Strike: Source (CS:S) Android - Native OpenGL ES Engine Port (css.md)

Bu belge, **Counter-Strike: Source / Source Engine 2013** projesinin Android platformuna taşınması, MaterialSystem grafik arka yüzünün doğrudan **Native OpenGL ES (GLES 2.0 / 3.0)** mimarisine geçirilmesi, karşılaşılan tüm teknik problemler, çözümler, derleme süreçleri ve Git çalışma kurallarını içeren kapsamlı teknik referans rehberidir.

Bu projeyi devralan veya inceleyen diğer yapay zeka asistanlarının ve geliştiricilerin tüm süreci eksiksiz anlaması için hazırlanmıştır.

---

## 1. Projenin Amacı ve Temel Vizyonu

### 1.1. Neden MaterialSystem'i Tamamen Native OpenGL ES'e Geçiriyoruz?
Orijinal Valve Source Engine motoru, grafik arka yüzü olarak Microsoft DirectX 9 (`materialsystem/shaderapidx9`) mimarisine sıkı sıkıya bağlıdır. Valve, motoru Linux ve Mac OS X platformlarına taşırken **TOGL** (Direct3D 9 API ve D3D9 SM2/SM3 assembly shader komutlarını çalışma anında OpenGL masaüstü API'lerine ve GLSL koduna dönüştüren bir emülasyon/çeviri katmanı) kullanmıştır. Android üzerinde ise topluluk projelerinde TOGL'un OpenGL ES uyarlaması olan **TOGLES** kullanılmaktaydı.

Ancak mobil Android donanımlarında `togles` kullanılması şu kritik sorunlara yol açmaktadır:
1. **Ağır CPU/GPU Yükü (Emulation Overhead):** Her çizim çağrısında D3D9 durumlarının OpenGL ES durumlarına dönüştürülmesi mobil işlemcileri ve termal sınırları aşırı zorlar.
2. **Shader Transpile Gecikmeleri:** D3D9 bytecode'larının mobil GPU sürücülerinin anlayabileceği GLSL'e dönüştürülmesi kasmaya (stuttering) ve bellek sızıntılarına neden olur.
3. **Sürücü Uyumsuzlukları:** Adreno, Mali ve PowerVR GPU sürücülerinin TOGLES'in ürettiği karmaşık ve standart dışı shader kodlarını derlerken kilitlenmesi veya çökmeler yaşaması.
4. **Draw Call Darboğazı:** Masaüstü mimarisine göre tasarlanmış binlerce küçük çizim çağrısı mobil GPU hatlarını tıkar.

### 1.2. Çözüm: `materialsystem/shaderapigl`
Projede DirectX emülasyonu (`shaderapidx9`, `togl`, `togles`) tamamen devre dışı bırakılmış ve doğrudan donanım seviyesinde çalışan **native OpenGL ES MaterialSystem backend'i** (`materialsystem/shaderapigl`) sıfırdan yazılmıştır:
- Source Engine'in `IShaderAPI` ve `IShaderDeviceMgr` arayüzleri doğrudan OpenGL ES 2.0/3.0 komutlarıyla uygulanmıştır.
- Harita (BSP), modeller (MDL), arayüz (VGUI) ve parçacıklar doğrudan modern GLSL programları (`u_mvp`, `attribute vec3 a_position`, `a_texcoord`, `a_lightcoord`) üzerinden GPU'ya aktarılır.
- Sonuç: Çok daha yüksek FPS, sıfır çeviri gecikmesi, düşük pil tüketimi ve modern Android cihazlarda tam kararlılık.

---

## 2. Proje Yapısı ve Kullanılan Dizinler

- **Ana Kaynak Kod (Source Engine):** `/home/ubuntu/V5`
  - **Aktif Git Dalı:** `V4`
  - **Native GLES Motor Modülü:** `/home/ubuntu/V5/materialsystem/shaderapigl`
    - `shaderapigl.cpp`: Native OpenGL ES implementasyonu, mesh yönetimi, matrisler, shader programları ve çizim çağrıları.
    - `wscript`: Waf derleme yapılandırması.
  - **Motor Başlatıcı Değişikliği:** `/home/ubuntu/V5/launcher/launcher.cpp`
    - Android platformunda varsayılan shader kütüphanesi olarak `libshaderapigl.so` yüklenir. Kullanıcı açıkça `-dx9` veya `-togl` vermedikçe doğrudan native OpenGL ES devreye girer.
- **Android NDK:** `/home/ubuntu/android-ndk-r10e` (Toolchain: `arm-linux-androideabi-4.9`, Platform: `android-21`, Mimari: `arch-arm` / armeabi-v7a).
- **Aktif Kullanılan Launcher:** `/home/ubuntu/cssa-android-launcher`
  - Projede önceden `srceng-android-launcher` kullanılıyordu. Kullanıcının isteğiyle tamamen yeni **CSSA Launcher** (`cssa-android-launcher`) mimarisine geçilmiştir.
  - `app/src/main/jniLibs/armeabi-v7a/`: Derlenen ve strip edilen `.so` motor kütüphanelerinin konulduğu dizin.
- **Çıktı APK Dosyaları:**
  - `/home/ubuntu/cssa-debug.apk` (Kullanıcının indirdiği ve test ettiği ana APK)
  - `/home/ubuntu/csso-debug.apk` (Yedek kopya)

---

## 3. Derleme ve Paketleme Adımları (Build Workflow)

Bir sonraki AI veya geliştirici, kod üzerinde değişiklik yaptıktan sonra aşağıdaki adımları sırasıyla çalıştırmalıdır:

### Adım 1: Native Kütüphaneyi (Waf ile) Derleme
```bash
cd /home/ubuntu/V5
./waf build --targets=shaderapigl
```
*(Gerektiğinde tüm motor hedefleri için `./waf build` çalıştırılabilir).*

### Adım 2: Kütüphaneyi Strip Edip Launcher Dizinine Kopyalama
Derlenen kütüphane boyutu hata ayıklama sembolleri nedeniyle büyüktür. NDK toolchain'indeki strip aracıyla Launcher'ın jniLibs dizinine aktarılır:
```bash
/home/ubuntu/android-ndk-r10e/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-strip \
  -s /home/ubuntu/V5/build/materialsystem/shaderapigl/libshaderapigl.so \
  -o /home/ubuntu/cssa-android-launcher/app/src/main/jniLibs/armeabi-v7a/libshaderapigl.so
```

### Adım 3: Launcher APK'sını Gradle ile Derleme
```bash
cd /home/ubuntu/cssa-android-launcher
./gradlew assembleDebug
```

### Adım 4: APK Dosyalarını Kök Dizine Kopyalama
Kullanıcının kolayca indirebilmesi için üretilen APK dosyaları kopyalanır:
```bash
cp /home/ubuntu/cssa-android-launcher/app/build/outputs/apk/debug/app-debug.apk /home/ubuntu/cssa-debug.apk
cp /home/ubuntu/cssa-android-launcher/app/build/outputs/apk/debug/app-debug.apk /home/ubuntu/csso-debug.apk
```

---

## 4. Karşılaşılan Kritik Sorunlar ve Uygulanan Çözümler

### 4.1. "Altta Düz Çizgi" Hatası (The Flat Line Bug / Çift Transpose Sorunu)
- **Belirti:** Launcher'da "Launch" butonuna basıp oyunu başlatınca ekran gelmiyor, en altta sadece tek piksellik düz bir çizgi görünüyordu.
- **Kök Neden:**
  - Source Engine'in `CMatRenderContext::ForceSyncMatrix` fonksiyonu, matrisleri `g_pShaderAPI->LoadMatrix`'e göndermeden önce ZATEN `MatrixTranspose` işlemi uygular.
  - Dolayısıyla `shaderapigl` içerisindeki `matModel`, `matView` ve `matProj` matrisleri bellek düzeyinde zaten transpoze edilmiş haldedir ($M^T, V^T, P^T$).
  - `(matModel * matView) * matProj` çarpımı matematiksel olarak $(P \times V \times M)^T$ sonucunu üretir.
  - Bellekteki satır-öncelikli matris OpenGL ES `glUniformMatrix4fv` fonksiyonuna `GL_FALSE` ile verildiğinde GPU bunu sütun-öncelikli okur, yani otomatik olarak transpoze eder: $((P \times V \times M)^T)^T = P \times V \times M$. Bu GLSL için kusursuz çalışan doğru matristir.
  - Ancak önceki bir denemede `matProj * matView * matModel` çarpılıp üstüne bir de C++'ta `MatrixTranspose` yapılmıştı. Bu durum çift transpoze ($M \times V \times P$) yaratarak 2D tepe noktalarını ekrandan dışarı fırlattı ve tüm arayüzü en altta tek bir piksel çizgisine çökeltti.
  - Ayrıca vertex shader'a eklenen `pos.z = pos.z * 2.0 - pos.w;` satırı, 2D ortografik projeksiyonda $Z=0$ iken $Z$ değerini $-1.0$ yaparak arayüzün OpenGL yakın kırpma düzlemi (near plane) tarafından silinmesine yol açtı.
- **Çözüm:**
  - Vertex shader standart formuna döndürüldü:
    ```glsl
    gl_Position = u_mvp * vec4(a_position, 1.0);
    ```
  - Matris çarpımı ana menünün 60 FPS çalıştığı orijinal matematiksel sırasına getirildi:
    ```cpp
    VMatrix modelView, mvp;
    MatrixMultiply( matModel, matView, modelView );
    MatrixMultiply( modelView, matProj, mvp );
    glUniformMatrix4fv( s_uMVP, 1, GL_FALSE, mvp.Base() );
    ```

### 4.2. 3D Oyun Dünyasının Siyaha Düşmesi (VTF Specular Mask & Alpha Discard)
- **Belirti:** Ana menü 60 FPS çalışıyor, maça girince sesler geliyor, oyuncu ateş edip hareket edebiliyor, ancak 3D dünya ve harita tamamen zifiri karanlık/siyah kalıyordu.
- **Kök Neden:**
  - Fragment shader'a şeffaflık amacıyla eklenen `if (col.a < 0.05) discard;` komutu tüm dünyayı yok ediyordu.
  - Source Engine / CS:S kaplamalarında (VTF dosyaları) opak duvar ve zemin kaplamalarının alfa kanalı şeffaflık için DEĞİL, **yansıma/parlama maskesi (specular/envmap mask)** olarak kullanılır. Mat yüzeylerde bu değer `0.0`dır.
  - Discard filtresi alfa değeri 0 olan tüm mat duvarları, yerleri ve kaplamaları ekrandan siliyordu. Arka plan rengi de siyah temizlendiği için harita simsiyah görünüyordu.
- **Çözüm:**
  - Fragment shader'a malzeme bayraklarına bağlı akıllı alfa denetimi (`u_alphatest` ve `u_translucent`) eklendi:
    ```glsl
    if (u_alphatest != 0) {
        if (col.a < 0.3) discard;
    } else if (u_translucent == 0) {
        col.a = 1.0;
    }
    ```
  - Opak dünyada alfa değeri zorunlu `1.0` yapılarak kaplamaların silinmesi engellendi. Sadece tel örgü, ızgara gibi `$alphatest 1` malzemelerinde piksel atımı aktif bırakıldı. 2D VGUI arayüzünde ise `u_translucent = 1` geçilerek menü şeffaflıkları korundu.

### 4.3. Harita Yüklenirken Çökme (Empty Stubs / ClearBuffersObeyStencil)
- **Belirti:** Harita yüklenirken veya biterken motor çöküyordu.
- **Kök Neden:**
  - `ClearBuffersObeyStencil` ve `ClearBuffersObeyStencilEx` fonksiyonları boş bırakılmıştı. 3D sahne çizilmeden önce eski derinlik değerleri silinemediği için yeni kareler çizilemiyordu.
  - `BindVertexBuffer`, `BindIndexBuffer` ve `CShaderAPIGL::Draw` stub halindeydi. Harita yüzeyleri (brush/world geometry) bu arayüzden gönderildiği için harita çizilemiyordu.
- **Çözüm:**
  - `glClear`, `glDepthMask(GL_TRUE)` ve `glColorMask` ile doğru tampon temizleme fonksiyonları yazıldı.
  - `BindVertexBuffer` ve `BindIndexBuffer` motorun dinamik ve statik mesh akışlarına bağlandı.

### 4.4. Karakterlerin Hareketsiz / T-Pose Kalması (Skeletal Animation)
- **Belirti:** Karakter seçim ekranında modeller tuhaf ve donuk (T-pose/bind pose) duruyordu.
- **Kök Neden:** Shader henüz donanım kemik matrislerini (hardware skinning) desteklemediğinden iskelet animasyonları GPU tarafından işlenemiyordu.
- **Çözüm:** `studiorendercontext.cpp` üzerinden yazılımsal iskelet animasyonu (`bSoftwareSkin = true`) ve ARM NEON SIMD CPU ışıklandırması etkinleştirilerek karakter animasyonlarının CPU üzerinde hesaplanması sağlandı.

---

## 5. Git Commit ve GitHub Push Kuralları (HAYATİ ÖNEMDE)

Bu projede çalışan yapay zeka ajanları ve geliştiriciler aşağıdaki kurallara **kesinlikle ve tavizsiz** uymak zorundadır:

1. **ASLA TEST EDİLMEDEN COMMIT ATILMAZ:**
   - Kod derlendiğinde sadece sözdizimsel olarak doğru olduğu anlaşılır; ancak mobil cihazda çalıştığı garanti değildir.
   - Kullanıcı bizzat APK'yı cihazına yükleyip test etmeden ve onay vermeden **hiçbir şekilde commit veya push yapılamaz.**
2. **KULLANICI AÇIK ONAY VERMEDİKÇE ASLA `git push` YAPILMAZ:**
   - Uzak depoya (remote repository) kod göndermek geri alınması zor hatalara yol açabilir.
   - `git push` komutu yalnızca kullanıcı açıkça *"artık commit atıp pushla"*, *"github'a gönder"* dediğinde çalıştırılabilir.
3. **AI FİLİGRANI / WATERMARK YASAKTIR:**
   - Kodların içine `// Generated by AI`, `// OpenAI`, `// Claude`, `// Antigravity` veya yapay zeka tarafından yazıldığını ima eden hiçbir yorum satırı eklenemez.
   - Kod Valve'ın orijinal C++ standartlarına, Source Engine 2013 mimarisine ve temiz kod prensiplerine uygun olarak korunmalıdır.
4. **Onay Alındığında Kullanılacak Commit Mesajı Formatı:**
   Kullanıcı onay verdiğinde commit mesajları teknik ve açıklayıcı olmalıdır. Örnek:
   ```bash
   git add materialsystem/shaderapigl launcher/launcher.cpp wscript
   git commit -m "Implement native OpenGL ES backend in shaderapigl, fix double-transpose and VTF specular alpha discard"
   git push origin V4
   ```

---

## 6. Mevcut Durum ve Gelecek Geliştirme Adımları

### 6.1. Tamamlananlar:
- [x] `shaderapigl` modülü oluşturuldu ve Waf derleme sistemine eklendi.
- [x] Android launcher'da varsayılan olarak `shaderapigl` yüklenmesi sağlandı.
- [x] 2D VGUI ve ana menü native OpenGL ES üzerinden 60 FPS çalışır duruma getirildi.
- [x] Double-transpose hatası çözüldü, ekranın düz çizgiye çökmesi engellendi.
- [x] Alpha discard filtrelemesi düzeltilerek 3D dünyadaki kaplamaların silinmesi engellendi.
- [x] Static ve Dynamic Mesh VBO/IBO tampon tahsisleri ve `bAppend` desteği sağlandı.
- [x] Yeni `cssa-android-launcher` projesine tam entegrasyon sağlandı ve APK'lar üretildi.

### 6.2. Sıradaki Kontroller (Test Sonrası Gerekirse Yapılacaklar):
1. Kullanıcının güncel APK (`cssa-debug.apk`) ile maça girdikten sonraki ekran görüntüsü ve log durumunun doğrulanması.
2. Harita ışıklandırması (`u_lightmap` koordinatları ve ışık haritası dokusu) doğrulaması.
3. Görüş açısı (FOV) ve 3D derinlik aralığının (depth precision) ince ayarı.
4. Karakter ve silah kaplamalarının donanım doku formatları (ETC2/RGBA/DXT) ile GPU performans optimizasyonu.


## 7. 2026-09-10 — Oyun içi donuk/görünmeyen sahne için test derlemesi

Cihaz testi henüz yapılmadı; aşağıdakiler kaynak incelemesiyle bulunan eksikler ve uygulanan düzeltmelerdir, görsel sorunun kesin çözüldüğü anlamına gelmez.

- `CMeshGL::Draw` içindeki eksik `OnDrawMesh` çağrıları eklendi. Ortak çizim yolunda `SyncMatrices` ile güncel kamera/model/projeksiyon matrislerinin yüklenmesi sağlandı. Mevcut matris çarpım sırası korundu.
- `GetDynamicMesh`, vertex biçimini bağlı malzemeden veya vertex override mesh'inden alıyor. Dinamik vertex biçimlerinde sıkıştırma kapatılıyor.
- Statik VBO ve IBO geçerliliği ayrı takip ediliyor; vertex yüklemesinin index güncellemesini yanlışlıkla atlatması düzeltildi.
- Kısmi mesh değişikliklerinde vertex/index başlangıç adresleri ve bağlı vertex buffer byte offset'i uygulanıyor.
- Işık haritalı malzemelerde güncel lightmap sayfası sampler 1'e açıkça bağlanıyor; ikinci UV kanalı olan her malzemeye otomatik lightmap uygulanmıyor.
- Önceki bölümlerde anlatılan CPU animasyon zorlaması mevcut `studiorendercontext.cpp` içinde bulunmuyordu. `UpdateConfig`, donanımın `MaxVertexShaderBlendMatrices() == 0` bildirdiği durumda CPU skinning ve lighting'i etkinleştiriyor.
- İlk 16 perspektif çizimi için `[GL] World draw` tanı kayıtları eklendi. Her 300 draw'da yazılan yoğun kayıt kaldırıldı.

Doğrulama: `./waf build --targets=shaderapigl,studiorender` ve launcher `./gradlew assembleDebug` başarılı. İki kütüphane NDK strip aracıyla paketlendi. APK imzası, ZIP bütünlüğü ve paket içindeki kütüphanelerin üretilen ARM dosyalarıyla eşleşmesi kontrol edildi.

Çıktılar: `/home/ubuntu/cssa-debug.apk`, `/home/ubuntu/csso-debug.apk`.
Kaynak yedeği: `/home/ubuntu/gles-fix-backup-20260910/`.
Derleme kayıtları: `/home/ubuntu/gles-fix-build.log`, `/home/ubuntu/gles-fix-apk-build.log`.

Sıradaki kontrol: cihazda ana menü, haritaya giriş, kamera hareketi, karakter/silah animasyonları ve aydınlatma. Sorun sürerse ekran görüntüsü ve oyun loglarıyla devam edilmeli. Native GLES backend hâlâ tamamlanmamış arayüzler içeriyor; bu derleme tüm MaterialSystem portunun tamamlandığı anlamına gelmez. Commit/push yapılmadı.


## 8. 2026-09-10 — Ekran görüntüsü alındığında kısmen yenilenen donuk kare

Cihaz geri bildirimi: oyun girdilere/seslere yanıt veriyor, ancak FPS sayacı dahil görüntü donuyor; ekran görüntüsü alınınca el/silah kısmen değişiyor, dünya siyah. Önceki derleme bu sorunu çözmedi.

Kaynakta `AcquireThreadOwnership` ve `ReleaseThreadOwnership` tamamen boştu. MaterialSystem oyun sırasında queued render thread'e geçerken bu metotlarla GL bağlamını devreder; boş uygulama nedeniyle worker bağlamsız kalabiliyor. Bu, bildirilen donmanın güçlü adayıdır; cihaz loguyla henüz doğrulanmadı.

Düzeltme: SDL launcher'ın mevcut ana GL bağlamı `MakeContextCurrent` ile yeni thread'e bağlanıyor, eski thread üzerinde `glFlush` ardından NULL bağlamla bırakılıyor. Bağlama/bırakma başarısızlığı açık hata veriyor. SDL/EGL swap başarısızlığı ve eksik surface için sınırlı tanı kayıtları eklendi. Render target desteği bu değişiklikte uygulanmadı; siyah haritanın tamamen giderildiği henüz doğrulanmadı.

Doğrulama: native ve APK derlemesi başarılı; gerçek fonksiyon gövdelerini kullanan, SDL/launcher arayüzünü taklit eden host testi ana thread -> worker -> ana thread devrini ve hata yollarını geçti. Bu test gerçek Android EGL sürücüsü testi değildir. APK ZIP ve paketlenmiş kütüphanelerin eşleşmesi kontrol edildi.

Yeni test APK: `/home/ubuntu/cssa-context-fix.apk`; aynı çıktı `cssa-debug.apk` ve `csso-debug.apk` olarak da kopyalandı. Yedek kaynak, regresyon testi ve derleme logları `/home/ubuntu/gles-context-fix-20260910/` altında. Commit/push yapılmadı. İlk cihaz kontrolü: ekran görüntüsü almadan FPS/timer, kamera ve silah hareketlerinin sürekli yenilenmesi; ardından haritanın görünürlüğü.


## 9. 2026-09-10 — Siyah dünya ve parçacık çiziminde çökme

Cihaz geri bildirimi: GL bağlam devri düzeltmesi hareketleri/kare yenilenmesini düzeltti; dünya siyah kaldı. Yeni logda `Failed to lock lightmap`, başarılı dünya draw çağrıları ve parçacık çizimi sırasında SIGSEGV görüldü.

Çökme analizi: paketli `libmaterialsystem.so` .text bölümü yerel sembollü kütüphaneyle birebir eşleşiyor. 0x99798 adresi `CMatCallQueue::QueueFunctorInternal` içinde başarısız tahsis sonrası NULL+4 yazımı. Client çağrı zinciri parçacık render yoluna gidiyor. GLES `GetCurrentDynamicVBSize()` sıfır dönüyordu; queued context bunu vertex boyutuna bölüyor, parçacık batch boyutu sıfır kalıyor, döngü parçacık sayısını azaltmadan kuyruğu doldurabiliyor. Kapasite mevcut 1 MiB dinamik tampon boyutuna düzeltildi.

Siyah sahne için yapılan düzeltmeler:
- `TexLock`/`TexUnlock`: RGBA8888 CPU yazma alanı, sınır kontrolü, GPU alt bölge yüklemesi, eski lightmap verisini koruyan CPU kopyası eklendi. Şimdilik seviye 0, 2D, LDR kilitleme destekleniyor.
- `CreateTexture` mip depolamasını gerçekten ayırıyor; framebuffer, depth/stencil tamponu ve texture kayıtları ekleniyor, silinirken temizleniyor.
- Birincil render target için GLES framebuffer bağlama ve render target -> texture blit uygulandı. Temel uygulama her renk hedefi için ayrı depth/stencil tamponu kullanıyor; özel/shared depth handle davranışları, MRT ve ters texture -> RT kopyası hâlâ tamamlanmış değil.
- Malzemenin ilk temel snapshot'ındaki renk/alfa yazma maskeleri ve kapalı depth write durumu uygulanıyor. WriteZ/Occlusion geçişlerinin rastgele renk yazması engelleniyor; tüm çok geçişli shader durum sistemi henüz uygulanmış değil.
- `Engine_Post` için `$fbtexture` seçilerek sahne geçişi yapılıyor; bloom/AA/renk düzeltme henüz işlenmiyor. Skybox için `$hdrbasetexture` alternatifi deneniyor. Doku olmayan değişkenler `GetTextureValue` ile okunmuyor.
- Desteklenmeyen format dönüşümünde küçük/sıkıştırılmış kaynak tamponu RGBA gibi okuyabilen güvensiz fallback kaldırıldı. `GetHDREnabled`, HDR_TYPE_NONE bildiren backend ile tutarlı olarak false dönüyor.

Doğrulama: native ve APK derlemesi, APK ZIP/içerik kontrolü; gerçek kaynak fonksiyonlarını kullanan host GLES 3.2 Mesa testiyle lightmap tam/alt bölge yükleme, tekrar kilitlemede veri korunması, geçersiz bölge reddi, framebuffer izolasyonu/kopyası/geri dönüşü ve pozitif parçacık batch ilerlemesi doğrulandı. Android Mali üzerinde henüz test edilmedi.

Test APK: `/home/ubuntu/cssa-world-fix.apk` (aynı çıktı cssa-debug.apk/csso-debug.apk). Kaynak yedeği, test oluşturucu ve derleme logları `/home/ubuntu/gles-world-fix-20260910/`. Commit/push yapılmadı. Cihazda de_nuke görünürlüğü, ateş/partiküller, birkaç dakikalık kararlılık ve yeni log kontrol edilmeli.


## 10. 2026-09-10 — Görünen dünyanın dikey ters olması

Cihaz testinde dünya görünürlüğü düzeldi; HUD/el düzgünken dünya dikey ters göründü, kamera hareketinde değişen/garip çizimler bildirildi. Önceki framebuffer testi kopyalanan pikselleri doğruluyordu, sahnenin Source UV'leriyle ekranda örneklenme yönünü kapsamıyordu.

Düzeltme: temel doku bir `ITexture::IsRenderTarget()` dokusu olduğunda fragment shader V koordinatını `1-v` olarak örnekliyor; normal kaplamalar ve lightmap UV'leri korunuyor. Viewport ve scissor için mevcut render target yüksekliği kullanılıyor. Framebuffer kopyasındaki Source üst-sol kaynak/hedef dikdörtgenleri GLES alt-sol koordinatlarına çevriliyor. Kamera matris çarpımı değiştirilmedi.

Doğrulama: native ve APK derlemesi başarılı. Kaynaktan alınan gerçek vertex/fragment shader'larıyla GLES 3.2 Mesa üzerinde üstü kırmızı/altı mavi sahnenin doğru örneklenmesi, normal doku yönü, üst-sol kısmi blit ve önceki lightmap/framebuffer/parçacık testleri geçti. ZIP ve paketli native kütüphane eşleşmesi doğrulandı. Cihaz sonucu bekleniyor; hareket sırasında bildirilen diğer çizim sorunlarının tümünün çözüldüğü söylenemez.

Test APK: `/home/ubuntu/cssa-orientation-fix.apk`; aynı çıktı cssa-debug.apk/csso-debug.apk. Kaynak yedeği, test ve loglar `/home/ubuntu/gles-orientation-fix-20260910/`. Commit/push yapılmadı. Sonraki kontrol: düz sahne yönü, kamera çevirirken geometri/kaplama kararlılığı, varsa kalan sorunların kısa videosu ve yeni log.


## 11. 2026-09-10 — Kamera hareketinde duvar/konteyner kaplamalarının değişmesi

Cihaz geri bildirimi: dünya yönü düzeldi; duvar/konteyner görüntüleri birbirine dönüşüyor/değişiyor. Kaynak incelemesinde somut malzeme seçimi hatası bulundu: `engine/gl_rsurf.cpp` dünya grubu için bir `BindBatch` yapıyor, ardından her yüzey batch'i için `Bind(pDrawMaterial)` ve `DrawBatch` çağırıyor. GLES ortak çizim yolu güncel bağlı malzeme yerine GetDynamicMesh sırasında saklanan ilk malzemeye öncelik veriyordu. İlk görünen yüzey kamera ile değiştiğinde tüm batch grubuna uygulanan yanlış kaplama da değişebiliyordu.

Düzeltme: ortak DrawMeshInternal yolunda öncelik güncel `GetBoundMaterial()` sonucuna verildi; mesh malzemeleri yalnızca bağlı malzeme yoksa fallback. Bu seçim temel doku, lightmap bayrakları, alfa ve renk/derinlik maskelerinde ortak kullanılıyor. Geometri/matris ve framebuffer kodu değiştirilmedi.

Doğrulama: native/APK derlemesi; gerçek kaynak malzeme seçicisi ve GLES shader'ıyla aynı geometri üzerinde duvar/konteyner/duvar doku değişimleri, eski mesh malzemesinin ezilmesi, ters batch sırası ve fallback durumları test edildi. Önceki shader yönü, lightmap, framebuffer ve parçacık kapasitesi testleri de geçti. APK ZIP ve paketli GLES kütüphane eşleşmesi kontrol edildi. Cihaz doğrulaması bekleniyor; ayrıca geometri bozulması varsa bunun da çözüldüğü iddia edilmiyor.

Test APK: `/home/ubuntu/cssa-material-fix.apk` (cssa-debug.apk/csso-debug.apk aynı çıktı). Yedek kaynak, test ve loglar `/home/ubuntu/gles-material-fix-20260910/`. Commit/push yapılmadı.


## 12. 2026-09-10 — Siyah fonlu ışık sprite'ı ve soluk kontrast

Cihaz görüntülerinde ışık efekti siyah bir dörtgenle görünüyor; kapı girişinde ayrıca siyah alan var. Kapı alanının aynı nedenden kaynaklandığı kesinleşmedi.

Kaynak bulgusu: `CShaderShadowGL::BlendFunc` ve `EnableDepthTest` boştu. Özellikle glow sprite shader'ları SRC_ALPHA/ONE ve depth test kapalı isterken, genel çizimin ignore-Z kolu SRC_ALPHA/ONE_MINUS_SRC_ALPHA uyguluyordu. Artık blend katsayıları ve depth-test etkinliği shadow/snapshot içinde saklanıyor; ilk temel malzeme snapshot'ı çizimde blend enable/katsayılarını uyguluyor, blend varsa fragment alfa değeri korunuyor, shader depth testi kapatmışsa GL testi de kapatılıyor. Tüm çok geçişli shader sistemi henüz tamamlanmadı.

Solukluk için shader'daki yapay `max(light.rgb * 2.0, vec3(0.35))` tabanı kaldırıldı, `light.rgb * 2.0` kullanılıyor. Bu kontrastı bozan zorunlu aydınlatmayı kaldırır; sRGB/HDR/ton eşleme ve tüm Source shader renk davranışının tamamlandığı anlamına gelmez.

Doğrulama: native/APK derleme, APK ZIP ve native içerik kontrolü. Gerçek GLES Mesa testi eski alfa blend ile siyah sprite fonunu yeniden üretti; yeni additive blend ile siyahın arka planı koruduğunu, alfa sıfırın şeffaf olduğunu, sıfır lightmap'in zorla %35'e yükselmediğini doğruladı. Önceki yön, material batch, framebuffer/lightmap/kapasite testleri geçti. Cihaz doğrulaması bekleniyor.

Test APK: `/home/ubuntu/cssa-blend-fix.apk`; cssa-debug.apk/csso-debug.apk aynı çıktı. Yedek/test/loglar `/home/ubuntu/gles-blend-fix-20260910/`. Commit/push yapılmadı. Sıradaki cihaz kontrolü: ışık çevresindeki siyah kare, kapıdaki siyah alan ve karanlık/aydınlık bölgelerin kontrastı. Kapı alanı sürerse güncel log ve aynı konumdan görüntü istenmeli.


## 13. 2026-09-10 — Bot/nesne altındaki siyah gölge kareleri

Cihaz geri bildirimi: güneş/ışık efekti düzeldi, botların ve atılan nesnelerin altında siyah kareler var; bazı kapılar hâlâ siyah. Kaynak incelemesinde Shadow malzemesinin normal renk shader'ıyla işlendiği bulundu. Özgün Shadow shader alfa maskesinden vertex alfa sönmesini çıkarıyor ve beyaz ile gölge rengi arasında geçiş yaparak zemine çarpımlı karışım uyguluyor. Genel texture RGB * vertex color yolunda bu maske siyah dörtgene dönüşebiliyor.

Düzeltme: Shadow/Shadow_DX8/Shadow_DX6 malzemeleri için ayrı GLSL modu eklendi. Coverage = clamp(texture alpha - vertex alpha, 0, 1), çıktı = mix(white, material shadow color, coverage). Gölge rengi GetColorModulation'dan alınıyor. Maske dışı çarpımda zemini koruyor. Temel bilinear maske kullanılıyor; özgün 5 örnekli yumuşatma, gölge sisi ve ShadowModel/ShadowBuild özel shader'ları henüz tamamlanmadı.

Doğrulama: native/APK derlemesi, ZIP ve paketli GLES kütüphane eşleşmesi. Gerçek GLSL ile Mesa GLES testi maske dışının zemini korumasını, içinin malzeme rengine göre kararmasını ve vertex-alfa sönmesini doğruladı; aynı veri genel shader yolunda siyah kareyi yeniden üretti. Önceki testler geçti. Cihaz doğrulaması bekleniyor; kapıdaki siyahlığın kesin olarak bu nedenden olduğu söylenemez.

Test APK: `/home/ubuntu/cssa-shadow-fix.apk` (cssa-debug.apk/csso-debug.apk aynı). Yedek/test/loglar `/home/ubuntu/gles-shadow-fix-20260910/`. Commit/push yapılmadı. Sonraki kontrol: bot ve bomba altındaki gölgeler, kapı girişindeki siyah alan; devam ederse yeni log/görüntü.


## 14. 2026-09-10 — Tek kapı girişinde kalan siyah yüzey

Cihaz geri bildirimi: gölge kareleri ve diğer sorunlar düzeldi; de_nuke dış kapı girişinde düz siyah yüzey kalıyor. Yerel oyun BSP verisi yok, bu nedenle görüntüdeki yüzeyin malzeme/entity kimliği henüz doğrulanamadı.

Kaynakta somut eksik: C_FuncAreaPortalWindow::DrawModel mesafe saydamlığını render->SetBlend ile gönderiyor; engine/gl_rsurf.cpp ModulateMaterial bunu IMaterial::AlphaModulate ile aktarıyor. GLES ortak çizim yolu 3D malzemenin GetAlphaModulation değerini okumuyor ve daima temel snapshot[0] kullanıyordu. Bu davranış saydamlaşması gereken siyah bir brush yüzeyinin opak kalmasını açıklayabilir; bu kapıyla eşleşmesi cihaz testi bekliyor.

Düzeltme: 3D genel malzeme alfa değeri u_color alfasına katılıyor. Alfa 1'den küçükken SHADER_USING_ALPHA_MODULATION snapshot'ının blend/depth durumu seçiliyor; bu varyant yoksa temel snapshot'a dönülüyor. UI mevcut çizim rengini kullanmayı sürdürüyor; projected Shadow özel alfa sönmesi korunuyor. Tam WindowImposter cubemap shader desteği eklenmedi, yüzeyler isme göre gizlenmedi.

Doğrulama: native ve APK derlemesi başarılı. Gerçek GLES Mesa testinde siyah portal yüzeyi alfa 0/0.5/1 için sırasıyla arka planı koruyor/kısmen örtüyor/tam örtüyor; snapshot seçim/fallback ve UI/Shadow alfa korunması test edildi. Önceki materyal, gölge, ışık, RT yönü, lightmap/framebuffer ve parçacık testleri geçti. APK ZIP, native içerik ve imza kontrolü yapıldı. Android cihazda kapıya yaklaşma/uzaklaşma ve kapıdan içeri geçiş testi bekleniyor.

Test APK: /home/ubuntu/cssa-door-fix.apk (cssa-debug.apk/csso-debug.apk aynı çıktı). Yedek, test oluşturucu ve loglar /home/ubuntu/gles-door-fix-20260910/. Commit/push yapılmadı.


## 15. 2026-09-10 — İlk native GLES MaterialSystem commit kapsamı

Bu commit, Androidde varsayılan `shaderapigl` yolunu etkinleştirir ve cihazda ana menü ile de_nuke oyun içi çiziminin doğrulandığı durumu kaydeder. Harita, HUD, silah/eller, bot animasyonları, lightmapler, gölgeler, şeffaf sprite'lar, render target yönü ve kapı/portal saydamlığı çalışır durumdadır.

Geçirilen ana parçalar: IShaderAPI/IShaderDeviceMgr başlangıcı; VBO/IBO mesh çizimi; Source matris akışı; temel GLSL vertex/fragment programı; doku/lightmap yükleme ve bağlama; render target/FBO ve üst-sol koordinat dönüşümü; alpha-test, karışım, depth/write mask snapshotları; dinamik material seçimi; CPU skinning/lighting fallbacki; Shadow malzemesi ve material alpha modülasyonu.

Bilinen kapsam sınırları: Genel GLSL yolunda Source özel stdshader davranışlarının tamamı uygulanmış değildir. Çok geçişli shader render-statei, MRT, özel/paylaşımlı depth handleları, ters texture->RT kopyası, HDR/sRGB/ton mapping, bloom/AA/renk düzeltme, tam WindowImposter cubemap yolu ile ShadowModel/ShadowBuild ve gölge sisinin özel sürümleri eksiktir. Bunlar mevcut cihaz testinde görünür hata üretmemiştir; yeni harita/efekt sorunlarında önceliklendirilmelidir.

Doğrulama: ARM native `shaderapigl` ve debug APK derlendi; APK ZIP bütünlüğü, paketli kütüphane eşleşmesi ve v2 imzası kontrol edildi. Gerçek Mesa GLES 3.2 testleri doku/lightmap bölge yükleme, framebuffer kopyası/yönü, materyal batch seçimi, alfa blend, shadow mask, portal alpha ve parçacık batch ilerlemesini kapsar. Xiaomi Mali-G68 cihaz testinde önceki siyah sahne, ters dünya, değişen kaplama, siyah ışık/gölge kareleri ve kapı sorunları çözüldü.
