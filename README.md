# LogAnalyzer

Büyük log dosyalarını (cihaz logları, uygulama logları) satır satır okuyan, filtreleyen, istatistik çıkaran ve CSV/JSON olarak dışa aktaran bir komut satırı aracı. Qt 6 Widgets/GUI kullanmaz — saf `Qt6::Core` üzerine kurulu bir CLI aracıdır.

## Özellikler

| Özellik | Açıklama |
|---|---|
| Büyük dosya desteği | Dosya asla tamamen belleğe alınmaz, satır satır okunur — bellek kullanımı dosya boyutundan bağımsız sabit kalır |
| Yapılandırılabilir format | Log formatı koda sabitlenmemiştir; `--parser-pattern` ile herhangi bir formata uyarlanabilir |
| Zaman aralığı filtresi | `--from` / `--to` ile belirli bir zaman penceresine daralt |
| Seviye eşiği filtresi | `--level WARNING` gibi bir eşik ver, o seviye ve üstü gösterilir |
| Metin arama | `--search` ile mesaj içinde regex tabanlı arama |
| CSV / JSON dışa aktarma | `--export csv` ya da `--export json` ile sonucu dosyaya yaz |
| Excel uyumlu CSV | Noktalı virgül ayracı + UTF-8 BOM, Türkçe karakterler Excel'de bozulmadan açılır |
| Otomatik özet | Her çalıştırmada toplam/parse edilen/filtreyi geçen satır sayısı, seviyeye ve saate göre dağılım ekrana basılır |

## Mimari

```
LogAnalyzer/
├── CMakeLists.txt
├── main.cpp                    composition root
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
    └── cli/                    ── argüman katmanı ──
        └── CliOptions.h/.cpp   QCommandLineParser sarmalayıcısı
```

### Katmanlar ve SOLID

- **`core`** hiçbir şeye bağımlı değil; `export` ve `cli` yalnızca `core`'daki veri tiplerini okur. Bağımlılık her zaman `core`'a doğru işaret eder.
- Her önemli işlem önce bir **arayüz** (`ILogParser`, `ILogReader`, `IExporter`) olarak tanımlanır, sonra somut bir sınıf (`RegexLogParser`, `FileLogReader`, `CsvExporter`/`JsonExporter`) bu sözleşmeyi gerçekleştirir. Somut sınıflar yalnızca `main.cpp`'de (composition root) bir araya gelir — bu, **Dependency Inversion**'ın ve **Open/Closed** ilkesinin doğrudan uygulamasıdır: yeni bir format eklemek mevcut hiçbir sınıfı değiştirmeden mümkündür.
- Hiçbir yerde C++ exception kullanılmaz; her fallible işlem `bool` döner ve bir `QString &error` out-parametresini doldurur.

## Derleme

Qt 6.5+ ve CMake 3.19+ gerekir.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt6 kurulum yolu>
cmake --build build
```

Qt Creator kullanıyorsan projeyi açman ve normal şekilde derlemen (Ctrl+B) yeterli.

## Kullanım

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

# Farklı formatlı bir log için özel pattern
LogAnalyzer --file farkli-format.log \
    --parser-pattern "^(?<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3}) (?<level>\w+) \[[^\]]*\] [^:]+: (?<message>.*)$" \
    --timestamp-format "yyyy-MM-dd HH:mm:ss,zzz"
```

### Çıkış kodları

| Kod | Anlamı |
|---|---|
| `0` | Başarılı |
| `1` | Komut satırı / doğrulama hatası |
| `2` | Dosya okuma hatası |
| `3` | Dışa aktarma hatası |

## Bilinen sınırlar

- Çok satırlı log mesajları (ör. stack trace) desteklenmez, her fiziksel satır ayrı işlenir.
- Zaman damgaları yerel saat varsayılır, saat dilimi dönüşümü yapılmaz.
- Test framework'ü içermez.
- Tek dosya, tek thread — paralel okuma/işleme yoktur.
- gzip gibi sıkıştırılmış log dosyaları desteklenmez.
