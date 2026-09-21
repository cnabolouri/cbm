"use client";

import { useActionState } from "react";
import { Send, CheckCircle2, AlertCircle } from "lucide-react";
import { sendContactMessage, type ContactState } from "./actions";

const initialState: ContactState = { status: "idle", message: "" };

const fieldClass =
  "w-full bg-surface border border-divider px-3 py-2.5 text-[15px] text-ink caret-accent focus-visible:border-accent";

const labelClass =
  "block font-mono text-[10px] tracking-[0.08em] uppercase text-muted mb-1.5";

export function ContactForm() {
  const [state, formAction, pending] = useActionState(sendContactMessage, initialState);

  if (state.status === "success") {
    return (
      <div className="border border-accent bg-accent-100/50 p-6 flex items-start gap-3">
        <CheckCircle2 size={20} strokeWidth={1.5} className="text-accent-700 shrink-0 mt-0.5" />
        <p className="text-ink">{state.message}</p>
      </div>
    );
  }

  return (
    <form action={formAction} className="grid gap-5 max-w-[46ch]">
      <div>
        <label htmlFor="name" className={labelClass}>
          Name
        </label>
        <input id="name" name="name" type="text" required maxLength={120} className={fieldClass} />
      </div>

      <div>
        <label htmlFor="email" className={labelClass}>
          Email
        </label>
        <input id="email" name="email" type="email" required maxLength={200} className={fieldClass} />
      </div>

      <div>
        <label htmlFor="message" className={labelClass}>
          Message
        </label>
        <textarea
          id="message"
          name="message"
          required
          rows={6}
          maxLength={5000}
          className={`${fieldClass} resize-y`}
        />
      </div>

      {state.status === "error" && (
        <p
          aria-live="polite"
          className="flex items-start gap-2 text-[13px] text-ink-soft border border-divider p-3"
        >
          <AlertCircle size={16} strokeWidth={1.5} className="shrink-0 mt-0.5" />
          {state.message}
        </p>
      )}

      <button
        type="submit"
        disabled={pending}
        className="inline-flex items-center justify-center gap-2 font-heading text-[17px] font-semibold tracking-[0.03em] px-6 py-3 border bg-accent text-bg border-accent hover:bg-accent-600 disabled:opacity-45 transition-colors"
      >
        {pending ? "Sending…" : "Send message"}
        {!pending && <Send size={16} strokeWidth={1.5} />}
      </button>
    </form>
  );
}
