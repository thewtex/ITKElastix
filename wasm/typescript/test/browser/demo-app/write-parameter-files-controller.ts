// Generated file. To retain edits, remove this comment.

import * as elastix from '../../../dist/index.js'
import writeParameterFilesLoadSampleInputs, { usePreRun } from "./write-parameter-files-load-sample-inputs.js"

class WriteParameterFilesModel {
  inputs: Map<string, any>
  options: Map<string, any>
  outputs: Map<string, any>

  constructor() {
    this.inputs = new Map()
    this.options = new Map()
    this.outputs = new Map()
    }
}


class WriteParameterFilesController {

  constructor(loadSampleInputs) {
    this.loadSampleInputs = loadSampleInputs

    this.model = new WriteParameterFilesModel()
    const model = this.model

    if (loadSampleInputs) {
      const loadSampleInputsButton = document.querySelector("#writeParameterFilesInputs [name=loadSampleInputs]")
      loadSampleInputsButton.setAttribute('style', 'display: block-inline;')
      loadSampleInputsButton.addEventListener('click', async (event) => {
        loadSampleInputsButton.loading = true
        await loadSampleInputs(model)
        loadSampleInputsButton.loading = false
      })
    }

    // ----------------------------------------------
    // Inputs
    const parameterObjectElement = document.querySelector('#writeParameterFilesInputs input[name=parameter-object-file]')
    parameterObjectElement.addEventListener('change', async (event) => {
        const dataTransfer = event.dataTransfer
        const files = event.target.files || dataTransfer.files

        const arrayBuffer = await files[0].arrayBuffer()
        model.inputs.set("parameterObject", JSON.parse(new TextDecoder().decode(new Uint8Array(arrayBuffer))))
        const details = document.getElementById("writeParameterFiles-parameter-object-details")
        details.innerHTML = `<pre>${globalThis.escapeHtml(JSON.stringify(model.inputs.get("parameterObject"), globalThis.interfaceTypeJsonReplacer, 2))}</pre>`
        details.disabled = false
    })

    const parameterFilesElement = document.querySelector('#writeParameterFilesInputs sl-input[name=parameter-files]')
    parameterFilesElement.addEventListener('sl-change', (event) => {
        const values = parameterFilesElement.value.split(',').map(s => s.trim())
        model.inputs.set("parameterFiles", values)
    })

    // ----------------------------------------------
    // Options
    // ----------------------------------------------
    // Outputs
    const parameterFilesOutputDownload = document.querySelector('#writeParameterFilesOutputs sl-button[name=parameter-files-download]')
    parameterFilesOutputDownload.addEventListener('click', (event) => {
        event.preventDefault()
        event.stopPropagation()
        if (model.outputs.has("parameterFiles")) {
            model.outputs.get("parameterFiles").forEach((o) => globalThis.downloadFile(new TextEncoder().encode(o.data), o.path))
        }
    })

    const preRun = async () => {
      if (loadSampleInputs && usePreRun) {
        await loadSampleInputs(model, true)
        await this.run()
      }
    }

    const onSelectTab = async (event) => {
      if (event.detail.name === 'writeParameterFiles-panel') {
        const params = new URLSearchParams(window.location.search)
        if (!params.has('functionName') || params.get('functionName') !== 'writeParameterFiles') {
          params.set('functionName', 'writeParameterFiles')
          const url = new URL(document.location)
          url.search = params
          window.history.replaceState({ functionName: 'writeParameterFiles' }, '', url)
          await preRun()
        }
      }
    }

    const tabGroup = document.querySelector('sl-tab-group')
    tabGroup.addEventListener('sl-tab-show', onSelectTab)
    function onInit() {
      const params = new URLSearchParams(window.location.search)
      if (params.has('functionName') && params.get('functionName') === 'writeParameterFiles') {
        tabGroup.show('writeParameterFiles-panel')
        preRun()
      }
    }
    onInit()

    const runButton = document.querySelector('#writeParameterFilesInputs sl-button[name="run"]')
    runButton.addEventListener('click', async (event) => {
      event.preventDefault()

      if(!model.inputs.has('parameterObject')) {
        globalThis.notify("Required input not provided", "parameterObject", "danger", "exclamation-octagon")
        return
      }


      try {
        runButton.loading = true

        const t0 = performance.now()
        const { parameterFiles, } = await this.run()
        const t1 = performance.now()
        globalThis.notify("writeParameterFiles successfully completed", `in ${t1 - t0} milliseconds.`, "success", "rocket-fill")

        model.outputs.set("parameterFiles", parameterFiles)
        parameterFilesOutputDownload.variant = "success"
        parameterFilesOutputDownload.disabled = false
        const parameterFilesOutput = document.getElementById("writeParameterFiles-parameter-files-details")
        parameterFilesOutput.innerHTML = `<pre>${globalThis.escapeHtml(JSON.stringify(parameterFiles))}</pre>`
        parameterFilesOutput.disabled = false
      } catch (error) {
        globalThis.notify("Error while running pipeline", error.toString(), "danger", "exclamation-octagon")
        throw error
      } finally {
        runButton.loading = false
      }
    })
  }

  async run() {
    const options = Object.fromEntries(this.model.options.entries())
    const { parameterFiles, } = await elastix.writeParameterFiles(      this.model.inputs.get('parameterObject'),
      this.model.inputs.get('parameterFiles'),
      Object.fromEntries(this.model.options.entries())
    )

    return { parameterFiles, }
  }
}

const writeParameterFilesController = new WriteParameterFilesController(writeParameterFilesLoadSampleInputs)
