import type { Metadata } from "next";
import { Section } from "@/components/section";
import { ContactForm } from "./contact-form";

export const metadata: Metadata = {
  title: "Contact",
  description: "Questions about PM Node, the hardware, or the build — get in touch.",
};

export default function ContactPage() {
  return (
    <Section eyebrow="Contact" title="Get in touch" kicker="Questions, notes, corrections">
      <p className="max-w-[56ch] text-ink-soft mb-10">
        Questions about the hardware, the firmware, or how a particular
        measurement works — or you&apos;ve built something similar and want
        to compare notes. Either way, this reaches me directly.
      </p>
      <ContactForm />
    </Section>
  );
}
