# react-native-nitro-html-to-pdf

**A native HTML-to-PDF engine for React Native—without a WebView.** A shared C++ core powered by [Nitro Modules](https://nitro.margelo.com) lays out documents off the UI thread and returns a PDF in memory, as base64, or as a file.


| **Up to ~8.3× faster** | **1.8 ms** | **Zero-copy output** | **iOS + Android** |
|:--|:--|:--|:--|
| Measured against WebView-based alternatives | Median for a one-page receipt | `ArrayBuffer` over native PDF memory | Shared C++ layout and PDF engine |

```ts
import { NitroPdf } from 'react-native-nitro-html-to-pdf'

const pdf = await NitroPdf.generate({ html: '<h1>Hello, PDF</h1>' })

pdf.toArrayBuffer() // bytes, zero-copy
pdf.toBase64() // base64 string
await pdf.saveToFile(`${dir}/hello.pdf`) // file
```

## Features

- **Fast for real documents.** Android benchmarks show generation up to 2.5×–8.3× faster than `react-native-html-to-pdf` and `expo-print` across the tested documents. Results vary with document size and device; see the [benchmark details](#performance).
- **No browser lifecycle.** Generate PDFs without mounting a WebView or starting a browser process.
- **Choose your output.** Read a zero-copy `ArrayBuffer`, encode to base64, or write to a file—without requiring a temporary file for in-memory workflows.
- **Keep the UI responsive.** Layout and PDF writing run on a background thread.
- **Readable, searchable text.** Fonts are subset and embedded with Unicode mappings. Register TrueType or OpenType fonts with `registerFont()`.
- **Built for documents.** Page sizing and margins, PNG/JPEG images, tables, flexbox, gradients and rounded borders.
- **Shared native engine.** The same C++ layout and PDF-writing core runs on iOS and Android.
- **Works with your stack.** Preview with [react-native-pdf](https://github.com/wonday/react-native-pdf), upload with `fetch`, or save with your preferred file-system library.

## Installation

```sh
npm install react-native-nitro-html-to-pdf react-native-nitro-modules
cd ios && pod install
```

Requires the New Architecture (tested on React Native 0.86), iOS 15.1+ and Android 7.0+. With Expo, use a [development build](https://docs.expo.dev/develop/development-builds/introduction/); Expo Go can't load native code.

## Usage

### Use the output

```tsx
// Preview with react-native-pdf
;<Pdf
  source={{ uri: `data:application/pdf;base64,${pdf.toBase64()}` }}
  style={{ flex: 1 }}
/>

// Upload straight from memory
await fetch(url, { method: 'POST', body: pdf.toArrayBuffer() })

// Save to an absolute path or file:// URL
await pdf.saveToFile(`${documentsDir}/invoice.pdf`)
```

The PDF stays in native memory, so you can read it in any form, as often as you need.

### Page setup

```ts
await NitroPdf.generate({
  html,
  pageSize: 'Letter', // 'A4' (default) | 'Letter' | 'Legal'
  landscape: true,
  margins: { top: 72, right: 54, bottom: 72, left: 54 }, // points, 72 pt = 1 inch
})

await NitroPdf.generate({ html, width: 288, height: 432 }) // custom size: 4 x 6 in
```

Content flows across pages, and page breaks avoid splitting lines of text and images.

### Images and fonts

```ts
NitroPdf.registerFont({ family: 'Inter', path: '/path/to/Inter-Regular.ttf' })
NitroPdf.registerFont({
  family: 'Inter',
  path: '/path/to/Inter-Bold.ttf',
  weight: 700,
})

await NitroPdf.generate({
  html: '<img src="logo.png" width="120"><p style="font-family: Inter">Hello</p>',
  baseUrl: '/path/to/assets', // resolves relative image paths
})
```

Images can be PNG or JPEG, from local files or `data:` URIs; JPEGs are embedded without re-encoding. Without registered fonts, system fonts are used, and missing bold or italic faces are synthesized.

## Migrating

| react-native-html-to-pdf   | expo-print                   | react-native-nitro-html-to-pdf  |
| -------------------------- | ---------------------------- | ------------------------------- |
| `generatePDF({ html })`    | `printToFileAsync({ html })` | `NitroPdf.generate({ html })`   |
| `result.filePath`          | `result.uri`                 | `await pdf.saveToFile(path)`    |
| `result.base64`            | `result.base64`              | `pdf.toBase64()`                |
| `result.numberOfPages`     | `result.numberOfPages`       | `pdf.pageCount`                 |
| `width`, `height`          | `width`, `height`            | `width`, `height` or `pageSize` |
| `padding`, `paddingTop`, … | `margins` (iOS only)         | `margins` (both platforms)      |
| `baseURL`                  |                              | `baseUrl`                       |

Both libraries render with the system WebView, so they also run JavaScript and support whatever the browser supports. Check [HTML and CSS support](#html-and-css-support) before switching.

## API

### `NitroPdf.generate(options): Promise<PdfDocument>`

| Option            | Type                           | Default   | Description                           |
| ----------------- | ------------------------------ | --------- | ------------------------------------- |
| `html`            | `string`                       |           | HTML with inline `<style>` blocks.    |
| `baseUrl`         | `string`                       |           | Directory for relative image paths.   |
| `pageSize`        | `'A4' \| 'Letter' \| 'Legal'`  | `'A4'`    |                                       |
| `width`, `height` | `number`                       |           | Custom page size in points. Set both. |
| `margins`         | `{ top, right, bottom, left }` | `36` each | In points.                            |
| `landscape`       | `boolean`                      | `false`   |                                       |

### `NitroPdf.registerFont(font): void`

Registers a TrueType or OpenType file for CSS `font-family`: `{ family, path, weight = 400, italic = false }`.

### `PdfDocument`

| Member                    | Description                                       |
| ------------------------- | ------------------------------------------------- |
| `pageCount`, `byteLength` | Number of pages and size in bytes.                |
| `toArrayBuffer()`         | The bytes, wrapping native memory without a copy. |
| `toBase64()`              | The PDF as a base64 string.                       |
| `saveToFile(path)`        | Writes the PDF on a background thread.            |

## Performance

Median time to generate each PDF on Android, using identical HTML and file output for each library. In these tests, generation took **1.8 ms** for a one-page receipt and was up to **~8.3× faster** than the compared WebView-based libraries. iOS has not been benchmarked yet.

| Document | `react-native-nitro-html-to-pdf` | `react-native-html-to-pdf` | `expo-print` |
|:--|--:|--:|--:|
| Receipt · 1 page | **1.8 ms** | 15 ms · ~8.3× | 12 ms · ~6.7× |
| Invoice · 7 pages | **16 ms** | 76 ms · ~4.8× | 85 ms · ~5.3× |
| Invoice · 46 pages | **140 ms** | 346 ms · ~2.5× | 371 ms · ~2.7× |

Multipliers compare each library’s displayed median with this package’s median for the same document. Base64 and byte outputs show similar gains.

<details>
<summary>How this was measured</summary>

- Android 16 emulator (arm64) on an Apple M4 Pro; release builds, Hermes, React Native 0.86.
- react-native-html-to-pdf 1.3.0 and expo-print 57.0; A4 without margins.
- A warm-up run first, then the libraries alternate: 10 runs each (5 for the largest document).
- Timed in JavaScript with `performance.now()`, including the native round trip.
- Reproduce with the **Benchmark** tab of the [example app](example).

</details>

## HTML and CSS support

Layout is done by [litehtml](https://github.com/litehtml/litehtml) and covers what invoices, receipts, reports and tickets need:

- Block and inline layout, floats, positioning, tables, flexbox and lists
- Font family, weight and style, text decoration and `text-transform`
- Colors with transparency, linear and radial gradients, background images
- Solid, dashed and dotted borders with `border-radius`
- PNG and JPEG images, `@media print`, and `<title>` as the PDF title

Not supported yet: JavaScript, CSS grid, external stylesheets, network resources, `page-break-*`, repeating table headers, and scripts that need shaping, such as Arabic or Devanagari. If you need those, or Expo Go, a WebView-based library is the better fit.

## How it works

litehtml lays out the document once at the page width. Drawing commands are recorded and replayed page by page, with breaks moved so that text lines and images stay whole. A small PDF writer then emits PDF 1.7 with subset fonts, compressed streams and JPEG passthrough, and the result stays in native memory until you read it. Only the system font lookup is platform-specific; everything else is shared C++.

## Example app

```sh
git clone --recursive https://github.com/nitinshukla413/react-native-nitro-html-to-pdf.git
cd react-native-nitro-html-to-pdf && npm install
cd example && npm run android # or: npm run ios
```

## Contributing

Pull requests are welcome. litehtml and stb are git submodules (`git submodule update --init`). After changing [`src/specs`](src/specs), regenerate the bindings with `npx nitrogen`.

## License

MIT. Built on [litehtml](https://github.com/litehtml/litehtml) (BSD-3-Clause), [gumbo-parser](https://github.com/google/gumbo-parser) (Apache-2.0), [stb_image](https://github.com/nothings/stb) (MIT or public domain) and zlib.
