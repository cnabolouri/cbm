import type { Metadata } from "next";
import { Section } from "@/components/section";
import { DividerGrid, Cell } from "@/components/ui/grid";
import { Reveal } from "@/components/reveal";

export const metadata: Metadata = {
  title: "Status",
  description: "Where PM Node stands: hardware revisions, what's implemented, and what's next.",
};

const REVISIONS = [
  { rev: "Rev 01", note: "Display replaced — moved to the ST7796." },
  { rev: "Rev 02", note: "Joystick removed; pointer-driven navigation dropped." },
  { rev: "Rev 03", note: "KY-040 rotary encoder adopted as the single control." },
  { rev: "Rev 04", note: "Vibration sensor replaced with the IIS3DWB." },
  { rev: "Rev 05", note: "SPI bus conflicts resolved between the display, sensor and SD." },
];

const IMPLEMENTED = [
  "ST7796 TFT display init and landscape UI rendering",
  "Rotary encoder navigation with interrupt-based page switching",
  "Local Wi-Fi access point and web server",
  "IIS3DWB vibration sensing, live values and FFT pipeline",
  "MLX90640 thermal detection and display logic",
  "DS18B20 probe temperature with a dedicated page",
  "Sound acquisition with FFT support",
  "Recording workflow tied to the encoder button",
  "microSD detection and storage",
];

const PARTIAL = [
  "Final TFT page polish for all trend plots and labels",
  "Web and TFT parity for all page structures",
  "Mount-state-aware interpretation refinement",
  "Production-quality logging and error presentation",
];

const NEXT = [
  "Finalize TFT layouts for every live page",
  "Improve selective-redraw for smoother TFT updates",
  "Finalize web/TFT feature parity",
  "Document wiring, pin mappings and enclosure design",
];

export default function StatusPage() {
  return (
    <>
      <Section eyebrow="Hardware revisions, in order" title="Where it stands" kicker="Status">
        <DividerGrid className="grid-cols-1">
          {REVISIONS.map((r) => (
            <Cell key={r.rev} className="grid grid-cols-[90px_1fr] gap-4 items-baseline">
              <span className="font-mono text-[10px] text-accent-700">{r.rev}</span>
              <span>{r.note}</span>
            </Cell>
          ))}
        </DividerGrid>
      </Section>

      <section className="border-t border-divider">
        <div className="max-w-5xl mx-auto px-5 py-16">
          <Reveal>
            <div className="grid sm:grid-cols-3 gap-8">
              <div>
                <h3 className="font-heading text-[18px] mb-3 text-accent-700">Implemented</h3>
                <ul className="text-[13px] text-ink-soft space-y-2">
                  {IMPLEMENTED.map((i) => (
                    <li key={i} className="flex gap-2">
                      <span className="text-accent">—</span> {i}
                    </li>
                  ))}
                </ul>
              </div>
              <div>
                <h3 className="font-heading text-[18px] mb-3 text-muted">Partially implemented</h3>
                <ul className="text-[13px] text-ink-soft space-y-2">
                  {PARTIAL.map((i) => (
                    <li key={i} className="flex gap-2">
                      <span className="text-muted">—</span> {i}
                    </li>
                  ))}
                </ul>
              </div>
              <div>
                <h3 className="font-heading text-[18px] mb-3 text-muted">Next steps</h3>
                <ul className="text-[13px] text-ink-soft space-y-2">
                  {NEXT.map((i) => (
                    <li key={i} className="flex gap-2">
                      <span className="text-muted">—</span> {i}
                    </li>
                  ))}
                </ul>
              </div>
            </div>
          </Reveal>
        </div>
      </section>

      <section className="bg-deep text-bg">
        <div className="max-w-5xl mx-auto px-5 py-14">
          <Reveal>
            <p className="font-heading text-[22px] max-w-[52ch]">
              Not production ready yet. This is an advanced prototype — core
              sensing and interaction patterns work, but hardware
              robustness, calibration, documentation and validation still
              need work.
            </p>
          </Reveal>
        </div>
      </section>
    </>
  );
}
