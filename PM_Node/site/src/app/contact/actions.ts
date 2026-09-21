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

    return { status: "success", message: "Message sent — thanks, I'll get back to you." };
  } catch (err) {
    console.error("Contact form send threw:", err);
    return { status: "error", message: "Sending failed. Please try again in a moment." };
  }
}
