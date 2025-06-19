//import { PageProps } from "$fresh/server.ts";
import MarkdownIt from 'markdown-it';
import MarkdownItFootnote from 'markdown-it-footnote';

export default async function Greet({name}: {name: string}) {
  const realPath = Deno.cwd() + "/yttlj-continue/contents/Prolog/" + name;
  try {
      const txt = await Deno.readTextFile(realPath);
      const md = new MarkdownIt({html: true}).use(MarkdownItFootnote);
      const html = md.render(txt);
      return html;
  }
  catch (err) {
      console.log(`Slug handler for yttlj: ${err}`);
      return <div>Not found!<br/> {name}</div>;
  }
}
