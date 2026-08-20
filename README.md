# LogAnalyzer

Büyük log dosyalarını (cihaz logları, uygulama logları) satır satır okuyan, filtreleyen, istatistik çıkaran ve CSV/JSON olarak dışa aktaran bir araç. İki ayrı çalıştırılabilir olarak gelir ve ikisi de aynı `core/`/`export/` mantığını paylaşır:

- **`LogAnalyzer`** — saf `Qt6::Core` üzerine kurulu, komut satırı aracı.
- **`LogAnalyzerGui`** — `Qt6::Widgets` + `Qt6::Charts` ile yazılmış masaüstü arayüzü; çoklu dosya seçimi, sonuç tablosu, grafik ve son açılan dosyalar listesi içerir.

## Özellikler

| Özellik | Açıklama |
|---|---|
| Büyük dosya desteği | Dosya asla tamamen belleğe alınmaz, satır satır okunur — bellek kullanımı dosya boyutundan bağımsız sabit kalır |
| Yapılandırılabilir format | Log formatı koda sabitlenmemiştir; `--parser-pattern` (CLI) ile herhangi bir formata uyarlanabilir |
| Zaman aralığı filtresi | Belirli bir zaman penceresine daraltma |
| Seviye eşiği filtresi | Bir seviye eşiği verildiğinde o seviye ve üstü gösterilir |
| Metin arama | Mesaj içinde regex tabanlı arama |
| Çoklu dosya seçimi (GUI) | GUI'de birden fazla log dosyası birlikte seçilip aynı filtreyle taranabilir; sonuç tablosunda her satırın hangi dosyadan geldiği "Kaynak" sütununda görünür |
| Grafik (GUI) | Seviyeye göre dağılım bar grafiği ile gösterilir |
| Son açılan dosyalar (GUI) | Son açılan dosyalar kalıcı olarak (`QSettings`) hatırlanır |
| CSV / JSON dışa aktarma | Sonucu dosyaya yaz — her satırın kaynağı da (hangi dosyadan geldiği) dahil edilir |
| Excel uyumlu CSV | Noktalı virgül ayracı + UTF-8 BOM, Türkçe karakterler Excel'de bozulmadan açılır |
| Otomatik özet | Toplam/parse edilen/filtreyi geçen satır sayısı, seviyeye ve saate göre dağılım |

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
    │   ├── RegexLogParser.h/.cpp   regex tabanlı ayrıştırma
    │   ├── ILogReader.h         "satır satır oku" sözleşmesi
    │   ├── FileLogReader.h/.cpp    QFile/QTextStream ile streaming okuma
    │   ├── LogFilter.h/.cpp    zaman / seviye / arama kriterleri
    │   └── LogStats.h/.cpp     akış sırasında biriken istatistik
    ├── export/                 ── dışa aktarma katmanı ──
    │   ├── IExporter.h         "sonucu dosyaya yaz" sözleşmesi
    │   ├── CsvExporter.h/.cpp
    │   └── JsonExporter.h/.cpp
    ├── cli/                    ── argüman katmanı ──
    │   └── CliOptions.h/.cpp   QCommandLineParser sarmalayıcısı
    └── ui/                     ── GUI katmanı (yalnızca LogAnalyzerGui) ──
        ├── MainWindow.h/.cpp/.ui   ana pencere
        └── RecentFiles.h/.cpp      son açılan dosyalar listesi (QSettings ile kalıcı)
```

### Katmanlar ve SOLID

- **`core`** hiçbir şeye bağımlı değil; `export`, `cli` ve `ui` yalnızca `core`'daki veri tiplerini okur. Bağımlılık her zaman `core`'a doğru işaret eder.
- Her önemli işlem önce bir **arayüz** (`ILogParser`, `ILogReader`, `IExporter`) olarak tanımlanır, sonra somut bir sınıf (`RegexLogParser`, `FileLogReader`, `CsvExporter`/`JsonExporter`) bu sözleşmeyi gerçekleştirir. Somut sınıflar yalnızca composition root'larda (`main.cpp`, `main_gui.cpp`/`MainWindow`) bir araya gelir — bu, **Dependency Inversion**'ın ve **Open/Closed** ilkesinin doğrudan uygulamasıdır: yeni bir format eklemek mevcut hiçbir sınıfı değiştirmeden mümkündür.
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
2. Aranacak kelime/regex, parser pattern, zaman damgası formatı ve minimum seviye alanları CLI'deki `--search`/`--parser-pattern`/`--timestamp-format`/`--level` ile birebir aynı işi görür.
3. **Ara** — seçilen tüm dosyalar sırayla okunur, aynı filtreden geçirilir; sonuç tablosunda her satırın hangi dosyadan geldiği "Kaynak" sütununda görünür, sağ tarafta seviyeye göre dağılım grafiği güncellenir.
4. **Dışa Aktar** — `.csv` ya da `.json` uzantısına göre `CsvExporter`/`JsonExporter` seçilip son arama sonucu (kaynak bilgisiyle birlikte) dosyaya yazılır.

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
| `--search <regex>` | Hayır | Mesaj içinde aranacak regex |
| `--parser-pattern <regex>` | Hayır | Log formatını değiştirir — `timestamp`/`level`/`message` adında yakalama grupları içermeli |
| `--timestamp-format <format>` | Hayır | `--parser-pattern` ile birlikte kullanılır, Qt tarih format string'i |
| `--export <csv\|json>` | Hayır* | Dışa aktarma formatı |
| `--output <yol>` | Hayır* | Dışa aktarma dosyası yolu |

\* `--export` ve `--output` birlikte kullanılmalı — biri verilirse diğeri de gerekir.

Varsayılan log formatı:
```
[2026-08-12 06:05:02] WARNING: Odeme servisi yaniti gecikti, sure_ms=4310
```
Regex: `^\[(?<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\]\s+(?<level>\w+):\s*(?<message>.*)$`, zaman formatı `yyyy-MM-dd HH:mm:ss`.

### Örnekler

```bash
# Tüm dosyayı özetle, filtre yok
LogAnalyzer --file uygulama.log

# Sadece WARNING ve üstünü CSV'ye aktar
LogAnalyzer --file uygulama.log --level WARNING --export csv --output ozet.csv

# Belirli bir zaman aralığında "baglanti" geçen satırları JSON'a aktar
LogAnalyzer --file uygulama.log --from 2026-08-12T06:00:00 --to 2026-08-12T08:00:00 \
    --search "baglanti" --export json --output sonuc.json

# Farklı formatlı, gerçek bir log dosyası için özel pattern
# (bu ornek, logpai/loghub'daki gercek bir Hadoop cluster logu ile test edildi:
#  https://github.com/logpai/loghub/blob/master/Hadoop/Hadoop_2k.log)
LogAnalyzer --file farkli-format.log \
    --parser-pattern "^(?<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3}) (?<level>\w+) \[[^\]]*\] [^:]+: (?<message>.*)$" \
    --timestamp-format "yyyy-MM-dd HH:mm:ss,zzz"
```

`--parser-pattern` esnekliği yalnızca teoride değil, gerçek verilerle doğrulandı: `havayolu-sample.log` örneği (varsayılan format) ve [logpai/loghub](https://github.com/logpai/loghub)'dan alınan gerçek bir Hadoop cluster logu (1999 satır, tamamen farklı format) — kod hiç değişmeden, sadece bu iki argümanla iki farklı formatı da doğru ayrıştırdı. Aynı dosyalar GUI'den de (çoklu dosya seçimiyle birlikte) açılıp test edilebilir.

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
- Unit test framework'ü içermez.
- Tek thread — paralel okuma/işleme yoktur. CLI zaten tek dosya işler; GUI'de çoklu dosyalar da sırayla (paralel değil) okunur.
- GUI'de çoklu dosya taramasında bir dosya açılamazsa, o ana kadar okunan diğer dosyaların sonuçları da gösterilmeden arama iptal olur.
- gzip gibi sıkıştırılmış log dosyaları desteklenmez.
