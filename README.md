# tauchain

For more information about tauchain see http://idni.org

To build, run `make` or `make debug` for verbose debug prints.

Command line usage:
```bash
  tau help <command>      Prints usage of <command>.
  tau <command> [<args>]  Run <command> with <args>.
  Available commands:
  convert                 Convert JSON-LD to quads including all dependent algorithms.
  expand                  Run expansion algorithm http://www.w3.org/TR/json-ld-api/#expansion-algorithms including all dependant algorithms.
  nodemap                 Run JSON-LD node map generation algorithm.
  prove                   Run a query against a knowledgebase.
  toquads                 Run JSON-LD->RDF algorithm http://www.w3.org/TR/json-ld-api/#deserialize-json-ld-to-rdf-algorithm of already-expanded input.
```
