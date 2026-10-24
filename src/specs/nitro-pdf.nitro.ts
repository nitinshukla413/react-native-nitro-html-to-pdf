import type { HybridObject } from 'react-native-nitro-modules'

export type PageSize = 'A4' | 'Letter' | 'Legal'

/** Margins in PDF points (1pt = 1/72 inch). */
export interface PageMargins {
  top: number
  right: number
  bottom: number
  left: number
}

export interface PdfOptions {
  html: string
  /** Directory (absolute path or file:// URL) used to resolve relative image paths. */
  baseUrl?: string
  /** Defaults to 'A4'. Ignored when `width` and `height` are set. */
  pageSize?: PageSize
  /** Custom page width in points. Must be set together with `height`. */
  width?: number
  /** Custom page height in points. Must be set together with `width`. */
  height?: number
  /** Defaults to 36pt (0.5 inch) on every side. */
  margins?: PageMargins
  landscape?: boolean
}

export interface FontOptions {
  /** The name to use in CSS `font-family`. */
  family: string
  /** Absolute path or file:// URL of a TrueType/OpenType font file. */
  path: string
  /** CSS font weight from 1 to 1000. Defaults to 400. */
  weight?: number
  /** Defaults to false. */
  italic?: boolean
}

export interface PdfDocument extends HybridObject<{
  ios: 'c++'
  android: 'c++'
}> {
  readonly pageCount: number
  readonly byteLength: number
  /** Wraps the native PDF bytes without copying. */
  toArrayBuffer(): ArrayBuffer
  /** The PDF as a base64 string, e.g. for a `data:application/pdf;base64,` URI. */
  toBase64(): string
  /** Writes the PDF to an absolute path or file:// URL. */
  saveToFile(path: string): Promise<void>
}

export interface NitroPdf extends HybridObject<{ ios: 'c++'; android: 'c++' }> {
  /** Lays out and renders the HTML on a background thread. */
  generate(options: PdfOptions): Promise<PdfDocument>
  /** Registers a font file for use via CSS `font-family`. */
  registerFont(font: FontOptions): void
}
