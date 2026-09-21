import type { Metadata } from "next";
import { RotateCw, RotateCcw, CircleDot } from "lucide-react";
import { Section } from "@/components/section";
import { DividerGrid, Cell } from "@/components/ui/grid";

export const metadata: Metadata = {
  title: "Use",
  description: "How PM Node is operated in the field: one rotary encoder, eight on-device pages.",
};

const GESTURES = [
  { icon: RotateCw, title: "Rotate clockwise", body: "Next page." },
  { icon: RotateCcw, title: "Rotate counter-clockwise", body: "Previous page." },
  { icon: CircleDot, title: "Press", body: "Start or stop recording." },
];

const PAGES = [
  "Home", "Vibration", "Vibration FFT", "Thermal",
  "Temperature", "Sound", "Sound FFT", "System",
];

export default function UsePage() {
  return (
    <Section eyebrow="One control" title="The joystick came out; a rotary went in" kicker="Use">
      <p className="max-w-[64ch] text-ink-soft mb-10">
        PM Node is meant to be used as a handheld or mountable inspection
        node. A rotary encoder replaces earlier joystick-driven pointer
        control and keeps the interface to three moves — no menus to hunt
        through with a gloved hand.
      </p>

      <div className="grid sm:grid-cols-3 gap-5 mb-10">
        {GESTURES.map(({ icon: Icon, title, body }) => (
          <div key={title} className="border border-divider p-5 grid gap-2">
            <Icon size={30} strokeWidth={1.5} className="text-accent" />
            <span className="font-heading text-[20px]">{title}</span>
            <span className="text-[13px] text-muted">{body}</span>
          </div>
        ))}
      </div>

      <span className="block font-mono text-[11px] tracking-[0.12em] uppercase text-accent-text mb-3">
        On the TFT
      </span>
      <DividerGrid className="grid-cols-2 sm:grid-cols-4">
        {PAGES.map((p) => (
          <Cell key={p} className="text-center font-mono text-[10px] tracking-[0.06em] uppercase text-muted">
            {p}
          </Cell>
        ))}
      </DividerGrid>
      <p className="mt-8 max-w-[64ch] text-ink-soft">
        The TFT displays dedicated pages for live measurements, trends, and
        FFT views. A local web UI mirrors the same data in a browser over the
        device&apos;s own access point — useful when a phone is easier to
        read than a 3.5&Prime; screen in bright shop light.
      </p>
    </Section>
  );
}
