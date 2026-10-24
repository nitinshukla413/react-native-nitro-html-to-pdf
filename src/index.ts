import { NitroModules } from 'react-native-nitro-modules'
import type { NitroPdf as NitroPdfSpec } from './specs/nitro-pdf.nitro'

export type {
  FontOptions,
  PageMargins,
  PageSize,
  PdfDocument,
  PdfOptions,
} from './specs/nitro-pdf.nitro'

export const NitroPdf =
  NitroModules.createHybridObject<NitroPdfSpec>('NitroPdf')
