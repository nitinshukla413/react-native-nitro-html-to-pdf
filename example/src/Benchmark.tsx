import React, { useEffect, useState } from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';
import * as Print from 'expo-print';
import ReactNativeBlobUtil from 'react-native-blob-util';
import { generatePDF } from 'react-native-html-to-pdf';
import { NitroPdf } from 'react-native-nitro-html-to-pdf';
import { invoiceHtml, receiptHtml } from './samples';

declare const performance: { now(): number };
declare function atob(data: string): string;

const DOCUMENTS = [
  { label: 'Receipt', html: receiptHtml, runs: 10 },
  { label: '150-row invoice', html: invoiceHtml(150), runs: 10 },
  { label: '1000-row invoice', html: invoiceHtml(1000), runs: 5 },
];
const OUTPUTS = ['File', 'Base64', 'Bytes'] as const;
type Output = (typeof OUTPUTS)[number];
type Generate = (html: string, name: string) => Promise<void>;
type Library = 'nitro' | 'htmlToPdf' | 'expoPrint';
type Row = { label: string } & Partial<Record<Library, number>>;

const dir = ReactNativeBlobUtil.fs.dirs.CacheDir;

function base64ToBytes(base64: string): Uint8Array {
  const binary = atob(base64);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

// Every library prints A4 (595 x 842 pt) without margins and produces the same artifact per output.
function nitro(output: Output): Generate {
  return async (html, name) => {
    const pdf = await NitroPdf.generate({
      html,
      pageSize: 'A4',
      margins: { top: 0, right: 0, bottom: 0, left: 0 },
    });
    if (output === 'File') await pdf.saveToFile(`${dir}/${name}.pdf`);
    if (output === 'Base64') pdf.toBase64();
    if (output === 'Bytes') new Uint8Array(pdf.toArrayBuffer());
  };
}

function htmlToPdf(output: Output): Generate {
  return async (html, name) => {
    // html-to-pdf rejects a call made right after the previous one resolves, so retry briefly.
    for (let attempt = 0; ; attempt++) {
      try {
        const result = await generatePDF({
          html,
          fileName: name,
          directory: 'benchmark',
          width: 595,
          height: 842,
          base64: output !== 'File',
        });
        if (output === 'Bytes') base64ToBytes(result.base64 ?? '');
        return;
      } catch (e) {
        if (attempt === 5) throw e;
        await new Promise<void>(resolve => setTimeout(resolve, 10));
      }
    }
  };
}

function expoPrint(output: Output): Generate {
  return async html => {
    const result = await Print.printToFileAsync({
      html,
      width: 595,
      height: 842,
      margins: { top: 0, right: 0, bottom: 0, left: 0 },
      base64: output !== 'File',
    });
    if (output === 'Bytes') base64ToBytes(result.base64 ?? '');
  };
}

const LIBRARIES: {
  key: Library;
  title: string;
  make: (output: Output) => Generate;
}[] = [
  { key: 'nitro', title: 'nitro-pdf', make: nitro },
  { key: 'htmlToPdf', title: 'html-to-pdf', make: htmlToPdf },
  { key: 'expoPrint', title: 'expo-print', make: expoPrint },
];

async function time(task: () => Promise<void>): Promise<number> {
  const start = performance.now();
  await task();
  return performance.now() - start;
}

function median(values: number[]): number {
  const sorted = [...values].sort((a, b) => a - b);
  return sorted[Math.floor(sorted.length / 2)];
}

function formatMs(ms?: number): string {
  if (ms === undefined) return '…';
  return ms < 10 ? `${ms.toFixed(1)} ms` : `${Math.round(ms)} ms`;
}

export function Benchmark() {
  const [output, setOutput] = useState<Output>('File');
  const [rows, setRows] = useState<Row[]>([]);
  const [status, setStatus] = useState('Warming up…');
  const [running, setRunning] = useState(true);

  useEffect(() => {
    // First calls pay one-off start-up costs (e.g. creating a WebView), so keep them out of the numbers.
    (async () => {
      for (const lib of LIBRARIES)
        await lib.make('File')(receiptHtml, 'warmup');
    })()
      .catch(e => setStatus(String(e)))
      .finally(() => {
        setRunning(false);
        setStatus('');
        console.log('BENCH_READY');
      });
  }, []);

  const run = async () => {
    setRunning(true);
    const results: Row[] = DOCUMENTS.map(doc => ({ label: doc.label }));
    setRows(results);
    try {
      for (const [index, doc] of DOCUMENTS.entries()) {
        setStatus(`Running: ${doc.label}`);
        const times: Record<Library, number[]> = {
          nitro: [],
          htmlToPdf: [],
          expoPrint: [],
        };
        // Alternate the libraries so all of them see the same device conditions.
        for (let i = 0; i <= doc.runs; i++) {
          for (const lib of LIBRARIES) {
            const generate = lib.make(output);
            const ms = await time(() => generate(doc.html, `doc-${index}`));
            if (i > 0) times[lib.key].push(ms);
          }
        }
        results[index] = {
          label: doc.label,
          nitro: median(times.nitro),
          htmlToPdf: median(times.htmlToPdf),
          expoPrint: median(times.expoPrint),
        };
        setRows([...results]);
        console.log(`BENCH_RESULT ${output} ${JSON.stringify(results[index])}`);
      }
      setStatus('');
      console.log('BENCH_DONE');
    } catch (e) {
      setStatus(String(e));
      console.log(`BENCH_ERROR ${String(e)}`);
    } finally {
      setRunning(false);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.heading}>HTML to PDF</Text>
      <Text style={styles.caption}>
        react-native-nitro-html-to-pdf, react-native-html-to-pdf and expo-print generate
        the same documents. Median time per PDF.
      </Text>
      <View style={styles.chips}>
        {OUTPUTS.map(option => (
          <Pressable
            key={option}
            disabled={running}
            onPress={() => setOutput(option)}
            style={[styles.chip, option === output && styles.chipActive]}
          >
            <Text
              style={[
                styles.chipText,
                option === output && styles.chipTextActive,
              ]}
            >
              {option}
            </Text>
          </Pressable>
        ))}
      </View>
      <Pressable
        disabled={running}
        onPress={run}
        style={[styles.button, running && styles.disabled]}
      >
        <Text style={styles.buttonText}>Run benchmark</Text>
      </Pressable>

      <View style={styles.table}>
        <View style={styles.row}>
          <Text style={[styles.cell, styles.first, styles.head]}>Document</Text>
          {LIBRARIES.map(lib => (
            <Text key={lib.key} style={[styles.cell, styles.head]}>
              {lib.title}
            </Text>
          ))}
        </View>
        {rows.map(row => (
          <View key={row.label} style={styles.row}>
            <Text style={[styles.cell, styles.first]}>{row.label}</Text>
            {LIBRARIES.map(lib => (
              <View key={lib.key} style={styles.cellBox}>
                <Text
                  style={[styles.value, lib.key === 'nitro' && styles.nitro]}
                >
                  {formatMs(row[lib.key])}
                </Text>
                {lib.key !== 'nitro' && row.nitro && row[lib.key] ? (
                  <Text style={[styles.value, styles.ratio]}>
                    {`${(row[lib.key]! / row.nitro).toFixed(1)}x slower`}
                  </Text>
                ) : null}
              </View>
            ))}
          </View>
        ))}
      </View>
      {status ? <Text style={styles.caption}>{status}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: 'white' },
  heading: { fontSize: 22, fontWeight: '700', color: '#111' },
  caption: { fontSize: 13, color: '#555', marginTop: 4 },
  chips: { flexDirection: 'row', gap: 8, marginTop: 12 },
  chip: {
    paddingVertical: 6,
    paddingHorizontal: 12,
    borderRadius: 16,
    borderWidth: 1,
    borderColor: '#ccc',
  },
  chipActive: { backgroundColor: '#2563eb', borderColor: '#2563eb' },
  chipText: { fontSize: 14, color: '#333' },
  chipTextActive: { color: 'white' },
  button: {
    marginVertical: 12,
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    backgroundColor: '#2563eb',
  },
  buttonText: { color: 'white', fontWeight: '600', fontSize: 15 },
  disabled: { opacity: 0.4 },
  table: {
    borderWidth: StyleSheet.hairlineWidth,
    borderColor: '#ccc',
    borderRadius: 8,
    overflow: 'hidden',
  },
  row: { flexDirection: 'row', paddingVertical: 8, paddingHorizontal: 8 },
  cell: { flex: 1, fontSize: 13, color: '#222', textAlign: 'right' },
  cellBox: { flex: 1 },
  value: { fontSize: 13, color: '#222', textAlign: 'right' },
  first: { flex: 1.3, textAlign: 'left' },
  head: { fontWeight: '700' },
  nitro: { color: '#2563eb', fontWeight: '700' },
  ratio: { fontSize: 11, color: '#888' },
});
