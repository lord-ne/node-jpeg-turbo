import { NdArray } from "@types/ndarray";

export type Format = number;
export const FORMAT_RGB: Format;
export const FORMAT_BGR: Format;
export const FORMAT_RGBX: Format;
export const FORMAT_BGRX: Format;
export const FORMAT_XRGB: Format;
export const FORMAT_XBGR: Format;
export const FORMAT_GRAY: Format;
export const FORMAT_RGBA: Format;
export const FORMAT_BGRA: Format;
export const FORMAT_ABGR: Format;
export const FORMAT_ARGB: Format;

export type SubSampling = number;
export const SAMP_444: SubSampling;
export const SAMP_422: SubSampling;
export const SAMP_420: SubSampling;
export const SAMP_GRAY: SubSampling;
export const SAMP_440: SubSampling;

export interface BufferSizeOptions {
  width: number;
  height: number;
  subsampling?: SubSampling;
}

export interface EncodeOptions extends BufferSizeOptions {
  format: Format;
  stride?: number;
  quality?: number;
}

export function bufferSize(options: BufferSizeOptions): number;

export function compressSync(raw: Buffer, options: EncodeOptions): Buffer;
export function compressSync(raw: Buffer, preallocatedOut: Buffer, options: EncodeOptions): Buffer;

export function compress(raw: Buffer, options: EncodeOptions): Promise<Buffer>;
export function compress(
  raw: Buffer,
  preallocatedOut: Buffer,
  options: EncodeOptions
): Promise<Buffer>;

export interface DecodeOptions {
  format: Format;
}

export interface DecompressReturn {
  data: Buffer;
  width: number;
  height: number;
  size: number;
  format: any;
}

export function decompressSync(image: Buffer, preallocatedOut: Buffer, options?: DecodeOptions): DecompressReturn;
export function decompressSync(image: Buffer, options?: DecodeOptions): DecompressReturn;

export function decompress(
  image: Buffer,
  preallocatedOut: Buffer,
  options?: DecodeOptions
): Promise<DecompressReturn>;
export function decompress(image: Buffer, options?: DecodeOptions): Promise<DecompressReturn>;

export interface DCTComponent {
  data: NdArray<Int16Array>;
  qt_no: Number;
}

export interface DCTData {
  Y: DCTComponent;
  Cb: DCTComponent;
  Cr: DCTComponent;
  K?: DCTComponent;
  qts: Array<NdArray<Uint16Array>>;
}

export function readDCTSync(image: Buffer, preallocatedOut?: Buffer): DCTData;
export function readDCT(image: Buffer, preallocatedOut?: Buffer): Promise<DCTData>;

export function writeDCTSync(originalImage: Buffer, dctData: DCTData, preallocatedOut?: Buffer): Buffer;
export function writeDCT(originalImage: Buffer, dctData: DCTData, preallocatedOut?: Buffer): Promise<Buffer>;

