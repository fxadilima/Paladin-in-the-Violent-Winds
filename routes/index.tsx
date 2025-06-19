
import MarkdownIt from 'markdown-it';
import MarkdownItFootnote from 'markdown-it-footnote';

export default async function Home() {
  const realPath = Deno.cwd() + "/index.md";
  const txt = await Deno.readTextFile(realPath);
  const md = new MarkdownIt({html: true}).use(MarkdownItFootnote);
  const html = md.render(txt);
  return (
    <div class="w3-content" dangerouslySetInnerHTML={{__html: html}} />
  );
}
