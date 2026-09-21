"use server";

import { Resend } from "resend";

export type ContactState = {
  status: "idle" | "success" | "error";
  message: string;
};

export async function sendContactMessage(
  _prev: ContactState,
  formData: FormData
): Promise<ContactState> {
  const name = String(formData.get("name") ?? "").trim();
  const email = String(formData.get("email") ?? "").trim();
  const message = String(formData.get("message") ?? "").trim();

  if (!name || !email || !message) {
    return { status: "error", message: "Please fill in every field." };
  }
  if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
    return { status: "error", message: "That email address doesn't look right." };
  }
  if (message.length > 5000) {
    return { status: "error", message: "That message is too long — 5000 characters max." };
  }

  const apiKey = process.env.RESEND_API_KEY;
  const to = process.env.CONTACT_EMAIL_TO;
  const from = process.env.CONTACT_EMAIL_FROM;

  if (!apiKey || !to || !from) {
    console.error("Contact form is not configured: missing RESEND_API_KEY, CONTACT_EMAIL_TO or CONTACT_EMAIL_FROM.");
    return {
      status: "error",
      message: "The contact form isn't configured yet. Please email directly instead.",
    };
  }

  try {
    const resend = new Resend(apiKey);

    // The notification is the one that must land. If it fails, the sender is
    // told the message didn't go through.
    const { error } = await resend.emails.send({
      from,
      to,
      replyTo: email,
      subject: `PM Node — message from ${name}`,
      text: `From: ${name} <${email}>\n\n${message}`,
    });

    if (error) {
      console.error("Resend rejected the contact message:", error);
      return { status: "error", message: "Sending failed. Please try again in a moment." };
    }

    // The acknowledgement is best-effort: their message is already delivered,
    // so a failure here is logged but never surfaced as a failed submission.
    try {
      const { error: ackError } = await resend.emails.send({
        from,
        to: email,
        replyTo: to,
        subject: "Thanks — your message reached PM Node",
        text: acknowledgementText(name, message),
        html: acknowledgementHtml(name, message),
      });
      if (ackError) {
        console.error("Acknowledgement email was rejected:", ackError);
      }
    } catch (ackErr) {
      console.error("Acknowledgement email threw:", ackErr);
    }

    return {
      status: "success",
      message: "Message sent — a confirmation is on its way to your inbox, and I'll be in touch soon.",
    };
  } catch (err) {
    console.error("Contact form send threw:", err);
    return { status: "error", message: "Sending failed. Please try again in a moment." };
  }
}

function escapeHtml(value: string) {
  return value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function acknowledgementText(name: string, message: string) {
  return [
    `Hi ${name},`,
    "",
    "Thanks for getting in touch about PM Node — your message came through and I'll get back to you as soon as possible.",
    "",
    "For your records, here's what you sent:",
    "",
    message,
    "",
    "—",
    "PM Node · pm-node.sinabolouri.com",
    "You're receiving this because this address was used on the PM Node contact form.",
  ].join("\n");
}

function acknowledgementHtml(name: string, message: string) {
  // Everything here is escaped first — this echoes back attacker-controllable
  // input, so it must never be interpolated raw.
  const safeName = escapeHtml(name);
  const safeMessage = escapeHtml(message).replace(/\n/g, "<br>");

  // Inline styles and a system font stack: webfonts and stylesheets are
  // unreliable across mail clients.
  return `<!doctype html>
<html>
  <body style="margin:0;padding:24px;background:#f2f2f3;color:#1d1f20;font-family:'Barlow',Helvetica,Arial,sans-serif;font-size:15px;line-height:1.6;">
    <div style="max-width:560px;margin:0 auto;background:#f2f2f3;border:1px solid rgba(29,31,32,0.16);padding:28px;">
      <div style="font-family:'Barlow Condensed',Helvetica,Arial,sans-serif;font-size:11px;letter-spacing:0.14em;text-transform:uppercase;color:#416180;">
        PM Node
      </div>
      <h1 style="margin:10px 0 18px;font-family:'Barlow Condensed',Helvetica,Arial,sans-serif;font-size:26px;font-weight:600;line-height:1.15;color:#1d1f20;">
        Thanks, ${safeName} — your message came through.
      </h1>
      <p style="margin:0 0 18px;color:#424244;">
        I'll get back to you as soon as possible.
      </p>
      <div style="border-left:3px solid #5980a6;padding:12px 16px;background:#e9e9ea;color:#424244;font-size:14px;">
        ${safeMessage}
      </div>
      <p style="margin:24px 0 0;padding-top:16px;border-top:1px solid rgba(29,31,32,0.16);font-size:12px;color:#5d5d60;">
        PM Node · <a href="https://pm-node.sinabolouri.com" style="color:#416180;">pm-node.sinabolouri.com</a><br>
        You're receiving this because this address was used on the PM Node contact form.
      </p>
    </div>
  </body>
</html>`;
}
