import type { Metadata } from "next";
import { Section } from "@/components/section";
import { DividerGrid, Cell } from "@/components/ui/grid";
import { Tag } from "@/components/ui/tag";
import { Reveal } from "@/components/reveal";

export const metadata: Metadata = {
  title: "Hardware",
  description: "The six sensors and the board PM Node is built on.",
};

const SPECS = [
  { part: "IIS3DWB", label: "Vibration + FFT", desc: "Per-axis velocity, dominant frequency, harmonic detection." },
  { part: "MLX90640", label: "Thermal camera", desc: "32 × 24 at 2 Hz with hotspot tracking and threshold regions." },
  { part: "DS18B20", label: "Probe temperature", desc: "Contact reference the thermal delta is measured against." },
  { part: "I2S MIC", label: "Sound + spectrum", desc: "16 kHz level and 256-point FFT with a character classification." },
  { part: "ST7796 + KY-040", label: "On-device UI", desc: "480 × 320 TFT, eight pages, one rotary encoder." },
  { part: "microSD + AP", label: "Record & serve", desc: "CSV at 20 Hz, WAV audio, and a web UI over its own access point." },
];

const STACK = [
  "ESP32-S3", "Arduino / C++", "Arduino_GFX_Library", "ST7796 TFT", "KY-040",
  "IIS3DWB", "MLX90640", "DS18B20", "I2S microphone", "microSD", "Wi-Fi AP", "Local web UI",
];

export default function HardwarePage() {
  return (
    <>
      <Section eyebrow="The node" title="Six sensors, one board" kicker="Hardware" feature>
        <DividerGrid feature className="sm:grid-cols-2 lg:grid-cols-3">
          {SPECS.map((s) => (
            <Cell key={s.part} feature className="grid gap-1.5">
              <span className="font-mono text-[10px] text-feature-accent">{s.part}</span>
              <span className="font-heading text-[22px]">{s.label}</span>
              <span className="text-[13px] text-feature-muted">{s.desc}</span>
            </Cell>
          ))}
        </DividerGrid>
      </Section>

      <section className="border-t border-divider">
        <div className="max-w-5xl mx-auto px-5 py-16">
          <Reveal>
            <span className="block font-mono text-[11px] tracking-[0.12em] uppercase text-accent-text mb-4">
              Stack
            </span>
            <div className="flex flex-wrap gap-2">
              {STACK.map((s) => (
                <Tag key={s}>{s}</Tag>
              ))}
            </div>
          </Reveal>
        </div>
      </section>
    </>
  );
}
