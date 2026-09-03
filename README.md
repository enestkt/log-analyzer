# LogAnalyzer

Büyük log dosyalarını (cihaz logları, uygulama logları) satır satır okuyan, filtreleyen, istatistik çıkaran ve CSV/JSON olarak dışa aktaran bir araç. İki ayrı çalıştırılabilir olarak gelir ve ikisi de aynı `core/`/`export/` mantığını paylaşır:

- **`LogAnalyzer`** — saf `Qt6::Core` üzerine kurulu, komut satırı aracı.
- **`LogAnalyzerGui`** — `Qt6::Widgets` + `Qt6::Charts` ile yazılmış masaüstü arayüzü; çoklu dosya seçimi, tarih aralığı filtresi, sonuç tablosu, grafik ve son açılan dosyalar listesi içerir.

## Özellikler

| Özellik | Açıklama |
|---|---|
| Büyük dosya desteği | Dosya asla tamamen belleğe alınmaz, satır satır okunur — bellek kullanımı dosya boyutundan bağımsız sabit kalır |
| **Otomatik format algılama** | Log formatı elle belirtilmezse, dosyanın ilk birkaç satırına bakılıp hazır format kütüphanesinden (bkz. aşağıda) en uygun ayrıştırıcı otomatik seçilir — çoğu sistem/şirket için özel regex yazmaya gerek kalmaz |
| Yapılandırılabilir format | Otomatik algılama yetmezse `--parser-pattern` (CLI) / pattern kutusu (GUI) ile herhangi bir formata elle uyarlanabilir |
| Zaman aralığı filtresi | Belirli bir zaman penceresine daraltma (CLI: `--from`/`--to`, GUI: tarih aralığı seçici) |
| Seviye eşiği filtresi | Bir seviye eşiği verildiğinde o seviye ve üstü gösterilir |
| Metin arama | Mesaj içinde regex tabanlı arama; düz kelimelerde küçük yazım hatalarına (1-2 harf farkına) tolerans vardır |
| Çoklu dosya seçimi (GUI) | GUI'de birden fazla log dosyası birlikte seçilip aynı filtreyle taranabilir; farklı formatlı dosyalar bir arada seçilse bile her biri kendi formatına göre ayrı ayrı algılanır; sonuç tablosunda her satırın hangi dosyadan geldiği "Kaynak" sütununda görünür |
| Grafik (GUI) | Seviyeye göre dağılım bar grafiği ile gösterilir |
| Son açılan dosyalar (GUI) | Son açılan dosyalar kalıcı olarak (`QSettings`) hatırlanır |
| CSV / JSON dışa aktarma | Sonucu dosyaya yaz — her satırın kaynağı da (hangi dosyadan geldiği) dahil edilir |
| Excel uyumlu CSV | Noktalı virgül ayracı + UTF-8 BOM, Türkçe karakterler Excel'de bozulmadan açılır |
| Otomatik özet | Toplam/parse edilen/filtreyi geçen satır sayısı, seviyeye ve saate göre dağılım |

## Otomatik format algılama — hazır format kütüphanesi

Log formatı elle belirtilmediğinde (`--parser-pattern` verilmemişse / pattern kutusu boşsa), `ParserLibrary::detect(...)` dosyanın ilk ~20 satırını örnekleyip, aşağıdaki stratejilerden **hangisi en çok satırı doğru ayrıştırıyorsa onu** otomatik seçer:

| Strateji | Ne zaman devreye girer | Örnek |
|---|---|---|
| **`GenericHeuristicParser`** | Satırda bir zaman damgası **ve** bir seviye kelimesi (`TRACE`/`DEBUG`/`INFO`/`WARNING`/`ERROR`/`CRITICAL`) bulunabiliyorsa — Log4j/Logback tarzı çoğu kurumsal sistemin logu bu kalıba uyar | `[2026-08-12 06:05:02] WARNING: mesaj`, Hadoop/Zookeeper tarzı loglar |
| **`SyslogParser`** | Klasik syslog kalıbı (`Ay Gün SS:DD:SS host process[pid]: mesaj`) eşleşiyorsa | `Dec 10 06:55:46 LabSZ sshd[24200]: Invalid user webmaster ...` |

Hiçbiri iyi eşleşmezse (örn. yapısı tamamen farklı bir format), en genel/kapsamlı seçenek olan `GenericHeuristicParser`'a düşülür ve satırlar "ayrıştırılamayan" olarak sayılır — program çökmez, sadece o satırlar istatistikte görünür. Bu durumda `--parser-pattern` ile elle bir kalıp tanımlanabilir (aşağıya bakın).

Çoklu dosya seçiminde algılama **her dosya için ayrı ayrı** yapılır — farklı sistemlerin loglarını (örn. bir Hadoop logu + bir syslog dosyasını) aynı anda seçip taratabilirsin, her biri kendi formatına göre doğru okunur.

## Mimari

```
LogAnalyzer/
├── CMakeLists.txt
├── main.cpp                    CLI composition root
├── main_gui.cpp                GUI composition root
└── src/
    ├── core/                   ── mantık katmanı, hiçbir şeye bağımlı değil ──
    │   ├── LogLevel.h/.cpp      seviye enum'u + metin dönüşümü
    │   ├── LogEntry.h           tek bir log satırının verisi
    │   ├── ILogParser.h         "satırı ayrıştır" sözleşmesi
    │   ├── RegexLogParser.h/.cpp   elle verilen regex kalıbıyla ayrıştırma
    │   ├── GenericHeuristicParser.h/.cpp   zaman damgası + seviye kelimesini tahmin ederek ayrıştırma
    │   ├── SyslogParser.h/.cpp     klasik syslog formatını ayrıştırma
    │   ├── ParserLibrary.h/.cpp    yukarıdaki stratejiler arasından otomatik seçim
    │   ├── ILogReader.h         "satır satır oku" sözleşmesi
    │   ├── FileLogReader.h/.cpp    QFile/QTextStream ile streaming okuma
    │   ├── LogFilter.h/.cpp    zaman / seviye / arama (yazım toleranslı) kriterleri
    │   └── LogStats.h/.cpp     akış sırasında biriken istatistik
    ├── export/                 ── dışa aktarma katmanı ──
    │   ├── IExporter.h         "sonucu dosyaya yaz" sözleşmesi
    │   ├── CsvExporter.h/.cpp
    │   └── JsonExporter.h/.cpp
    ├── cli/                    ── argüman katmanı ──
    │   └── CliOptions.h/.cpp   QCommandLineParser sarmalayıcısı
    └── ui/                     ── GUI katmanı (yalnızca LogAnalyzerGui) ──
        ├── MainWindow.h/.cpp/.ui   ana pencere (tarih aralığı, çoklu dosya vb.)
        └── RecentFiles.h/.cpp      son açılan dosyalar listesi (QSettings ile kalıcı)
```

### Katmanlar ve SOLID

- **`core`** hiçbir şeye bağımlı değil; `export`, `cli` ve `ui` yalnızca `core`'daki veri tiplerini okur. Bağımlılık her zaman `core`'a doğru işaret eder.
- Her önemli işlem önce bir **arayüz** (`ILogParser`, `ILogReader`, `IExporter`) olarak tanımlanır, sonra somut bir sınıf bu sözleşmeyi gerçekleştirir. `ILogParser`'ın üç somut implementasyonu var (`RegexLogParser`, `GenericHeuristicParser`, `SyslogParser`) — yeni bir format desteği eklemek, mevcut hiçbir sınıfı değiştirmeden, sadece `ParserLibrary`'ye yeni bir aday eklemek demektir. Bu, **Dependency Inversion**'ın ve **Open/Closed** ilkesinin doğrudan uygulamasıdır.
- Somut sınıflar yalnızca composition root'larda (`main.cpp`, `main_gui.cpp`/`MainWindow`) bir araya gelir.
- Hiçbir yerde C++ exception kullanılmaz; her fallible işlem `bool` döner ve bir `QString &error` out-parametresini doldurur.

## Derleme

Qt 6.5+ ve CMake 3.19+ gerekir.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt6 kurulum yolu>
cmake --build build
```

Bu komut hem `LogAnalyzer` (CLI) hem `LogAnalyzerGui`'yi derler; tek birini istersen `--target LogAnalyzer` ya da `--target LogAnalyzerGui` ekle.

Qt Creator kullanıyorsan projeyi açman ve normal şekilde derlemen (Ctrl+B) yeterli; hangi çalıştırılabilirin başlatılacağını sol alttaki hedef seçiciden değiştirebilirsin.

## Kullanım — GUI (`LogAnalyzerGui`)

1. **Dosya(lar) Seç** — Ctrl/Shift ile birden fazla log dosyası seçilebilir; ya da "Son Açılan Dosyalar" listesinden tek bir dosyaya tıklanabilir.
2. Aranacak kelime/regex, parser pattern, zaman damgası formatı ve minimum seviye alanları CLI'deki `--search`/`--parser-pattern`/`--timestamp-format`/`--level` ile birebir aynı işi görür. Pattern kutusu **boş bırakılırsa** format otomatik algılanır (yukarıya bakın).
3. **Tarih aralığı** kutucuğunu işaretleyip başlangıç/bitiş tarihlerini seçerek belirli bir zaman penceresine daraltabilirsin.
4. **Ara** — seçilen tüm dosyalar sırayla okunur, aynı filtreden geçirilir; sonuç tablosunda her satırın hangi dosyadan geldiği "Kaynak" sütununda görünür, sağ tarafta seviyeye göre dağılım grafiği güncellenir.
5. **Dışa Aktar** — `.csv` ya da `.json` uzantısına göre `CsvExporter`/`JsonExporter` seçilip son arama sonucu (kaynak bilgisiyle birlikte) dosyaya yazılır.

## Kullanım — CLI (`LogAnalyzer`)

```
LogAnalyzer --file <yol> [seçenekler]
```

| Bayrak | Zorunlu mu | Açıklama |
|---|---|---|
| `--file <yol>` | Evet | Analiz edilecek log dosyası |
| `--from <zaman>` | Hayır | Başlangıç zamanı, ISO 8601 (`2026-08-12T06:00:00`) |
| `--to <zaman>` | Hayır | Bitiş zamanı, ISO 8601 |
| `--level <SEVIYE>` | Hayır | Minimum seviye eşiği: `TRACE`/`DEBUG`/`INFO`/`WARNING`/`ERROR`/`CRITICAL` |
| `--search <regex>` | Hayır | Mesaj içinde aranacak regex; düz kelimelerde yazım hatasına tolerans vardır |
| `--parser-pattern <regex>` | Hayır | Log formatını elle sabitler — verilmezse format otomatik algılanır. Verilirse `timestamp`/`level`/`message` adında yakalama grupları içermeli |
| `--timestamp-format <format>` | Hayır | `--parser-pattern` ile birlikte kullanılır, Qt tarih format string'i |
| `--export <csv\|json>` | Hayır* | Dışa aktarma formatı |
| `--output <yol>` | Hayır* | Dışa aktarma dosyası yolu |

\* `--export` ve `--output` birlikte kullanılmalı — biri verilirse diğeri de gerekir.

### Örnekler

```bash
# Format elle belirtilmedi -- otomatik algilanir, ozet ekranda "Algilanan log formati: ..." olarak gorunur
LogAnalyzer --file uygulama.log

# Sadece WARNING ve üstünü CSV'ye aktar
LogAnalyzer --file uygulama.log --level WARNING --export csv --output ozet.csv

# Belirli bir zaman aralığında "baglanti" geçen satırları JSON'a aktar
LogAnalyzer --file uygulama.log --from 2026-08-12T06:00:00 --to 2026-08-12T08:00:00 \
    --search "baglanti" --export json --output sonuc.json

# Otomatik algilamanin yetersiz kaldigi, tamamen sira disi bir format icin elle pattern
LogAnalyzer --file farkli-format.log \
    --parser-pattern "^(?<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3}) (?<level>\w+) \[[^\]]*\] [^:]+: (?<message>.*)$" \
    --timestamp-format "yyyy-MM-dd HH:mm:ss,zzz"
```

Otomatik algılama gerçek, birbirinden çok farklı verilerle doğrulandı — hepsi repo kökünde örnek olarak duruyor:

| Dosya | Kaynak | Format | Sonuç |
|---|---|---|---|
| `havayolu-sample.log` | elle üretilmiş örnek | Köşeli parantezli, varsayılan tarzı | 54/55 satır ayrıştırıldı |
| `Hadoop_2k.log` | [logpai/loghub](https://github.com/logpai/loghub/blob/master/Hadoop/Hadoop_2k.log) | Thread adı + sınıf adı içeren Log4j tarzı | 2000/2000 |
| `Zookeeper_2k.log` | [logpai/loghub](https://github.com/logpai/loghub/tree/master/Zookeeper) | Parantez içinde IPv6 benzeri adresler içeren, iç içe yapılı | 2000/2000 |
| `OpenSSH_2k.log` | [logpai/loghub](https://github.com/logpai/loghub/tree/master/OpenSSH) | Klasik syslog (yıl ve seviye kelimesi yok) | 2000/2000 (`SyslogParser` ile) |

Dördünde de kod hiç değişmedi, hatta çoğunda elle pattern bile verilmedi — `ParserLibrary` doğru stratejiyi kendisi seçti. Aynı dosyalar GUI'den de (çoklu dosya seçimiyle birlikte, farklı formatlı dosyalar bir arada) açılıp test edilebilir.

### Çıkış kodları (CLI)

| Kod | Anlamı |
|---|---|
| `0` | Başarılı |
| `1` | Komut satırı / doğrulama hatası |
| `2` | Dosya okuma hatası |
| `3` | Dışa aktarma hatası |

## Bilinen sınırlar

- Çok satırlı log mesajları (ör. stack trace) desteklenmez, her fiziksel satır ayrı işlenir.
- Zaman damgaları yerel saat varsayılır, saat dilimi dönüşümü yapılmaz.
- Syslog formatında yıl bilgisi hiç yazılmadığı için (`SyslogParser`) dosyanın son değiştirilme yılı varsayılır (yoksa içinde bulunulan yıl) — bir dosyada yıl sınırını aşan (ör. Aralık→Ocak) kayıtlar varsa hâlâ yanılabilir.
- Otomatik format kütüphanesi şu an iki stratejiden oluşuyor (`GenericHeuristicParser`, `SyslogParser`); ikisi de tanıyamazsa sonuç ekranında "Bilinmiyor (varsayılan kullanılıyor, sonuçlar hatalı olabilir)" diye açıkça belirtilir. JSON satır satır loglar (`{"timestamp":...}` tarzı) ve tamamen sıra dışı yapılar için hâlâ elle `--parser-pattern` gerekir.
- `GenericHeuristicParser`, zaman damgasını satırın ilk 40, metadata kapanışını (`]`) ilk 100 karakterinde arar — bu, gerçek değeri mesajın derinlerinde geçen (çok uzun) satırlarda nadiren yanlış eşleşmeyi önlemek için bilinçli bir sınır.
- Unit test framework'ü içermez.
- Tek thread — paralel okuma/işleme yoktur. CLI zaten tek dosya işler; GUI'de çoklu dosyalar da sırayla (paralel değil) okunur.
- GUI'de çoklu dosya taramasında bir dosya açılamazsa, o ana kadar okunan diğer dosyaların sonuçları da gösterilmeden arama iptal olur.
- gzip gibi sıkıştırılmış log dosyaları desteklenmez.
