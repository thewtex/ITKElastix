// Generated file. To retain edits, remove this comment.

import {
  JsonCompatible,
  InterfaceTypes,
  PipelineOutput,
  PipelineInput,
  runPipelineNode
} from 'itk-wasm'

import ReadParameterFilesOptions from './read-parameter-files-options.js'
import ReadParameterFilesNodeResult from './read-parameter-files-node-result.js'


import path from 'path'

/**
 * Read elastix parameter text files into a parameter object.
 *
 * @param {ReadParameterFilesOptions} options - options object
 *
 * @returns {Promise<ReadParameterFilesNodeResult>} - result object
 */
async function readParameterFilesNode(
  options: ReadParameterFilesOptions = { parameterFiles: [] as string[], }
) : Promise<ReadParameterFilesNodeResult> {

  const mountDirs: Set<string> = new Set()

  const desiredOutputs: Array<PipelineOutput> = [
    { type: InterfaceTypes.JsonCompatible },
  ]

  const inputs: Array<PipelineInput> = [
  ]

  const args = []
  // Inputs
  // Outputs
  const parameterObjectName = '0'
  args.push(parameterObjectName)

  // Options
  args.push('--memory-io')
  if (typeof options.parameterFiles !== "undefined") {
    if(options.parameterFiles.length < 1) {
      throw new Error('"parameter-files" option must have a length > 1')
    }
    args.push('--parameter-files')

    options.parameterFiles.forEach((value) => {
      mountDirs.add(path.dirname(value as string))
      args.push(value as string)
    })
  }

  const pipelinePath = path.join(path.dirname(import.meta.url.substring(7)), '..', 'pipelines', 'read-parameter-files')

  const {
    returnValue,
    stderr,
    outputs
  } = await runPipelineNode(pipelinePath, args, desiredOutputs, inputs, mountDirs)
  if (returnValue !== 0) {
    throw new Error(stderr)
  }

  const result = {
    parameterObject: outputs[0].data as JsonCompatible,
  }
  return result
}

export default readParameterFilesNode