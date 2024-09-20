package io.github.hypengw.request;

import org.openapitools.codegen.*;
import org.openapitools.codegen.model.*;
import org.openapitools.codegen.languages.AbstractCppCodegen;
import org.openapitools.codegen.utils.ModelUtils;
import io.swagger.models.properties.*;
import io.swagger.v3.oas.models.media.Schema;
import io.swagger.v3.parser.util.SchemaTypeUtil;
import org.apache.commons.lang3.StringUtils;

import java.util.*;
import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.regex.Pattern;

public class CppRequestGenerator extends AbstractCppCodegen {
  protected String apiVersion = "1.0.0";
  protected final String PREFIX = "OAI";
  protected Map<String, String> systemIncludes = new HashMap<String, String>();
  protected Set<String> foundationClasses = new HashSet<>();
  public static final String DEFAULT_PACKAGE_NAME = "CppRequestOAIClient";

  /**
   * Configures the type of generator.
   *
   * @return the CodegenType for this generator
   * @see org.openapitools.codegen.CodegenType
   */
  public CodegenType getTag() {
    return CodegenType.OTHER;
  }

  /**
   * Configures a friendly name for the generator. This will be used by the
   * generator
   * to select the library with the -g flag.
   *
   * @return the friendly name for the generator
   */
  public String getName() {
    return "cpp-request";
  }

  public String getNamespace() {
    if (additionalProperties.containsKey("namespace")) {
      return additionalProperties.get("namespace").toString();
    }
    return "cpp_request";
  }

  public Path getIncludeFolder() {
    return Path.of("include", getNamespace());
  }

  public Path getSourceFolder() {
    return Path.of("src");
  }

  public Path getApiIncludeFolder() {
    return getIncludeFolder().resolve("api");
  }

  public Path getModelIncludeFolder() {
    return getIncludeFolder().resolve("model");
  }

  public Path getApiDstFromTemplateName(String tpn) {
    String suffix = apiTemplateFiles().get(tpn);
    if (suffix == ".h" || suffix == ".hpp")
      return getApiIncludeFolder();
    else
      return getSourceFolder().resolve("api");
  }

  public Path getModelDstFromTemplateName(String tpn) {
    String suffix = modelTemplateFiles().get(tpn);
    if (suffix == ".h" || suffix == ".hpp")
      return getModelIncludeFolder();
    else
      return getSourceFolder().resolve("model");
  }

  public String getPackageName() {
    if (additionalProperties.containsKey(CodegenConstants.PACKAGE_NAME)) {
      return additionalProperties.get(CodegenConstants.PACKAGE_NAME).toString();
    }
    return "cpp-request";
  }

  public String getApiNamePrefix() {
    if (additionalProperties.containsKey(CodegenConstants.API_NAME_PREFIX)) {
      return additionalProperties.get(CodegenConstants.API_NAME_PREFIX).toString();
    }
    return PREFIX;
  }

  public String getModelNamePrefix() {
    if (additionalProperties.containsKey(CodegenConstants.MODEL_NAME_PREFIX)) {
      return additionalProperties.get(CodegenConstants.MODEL_NAME_PREFIX).toString();
    }
    return PREFIX;
  }

  /**
   * Provides an opportunity to inspect and modify operation data before the code
   * is generated.
   */
  @Override
  public OperationsMap postProcessOperationsWithModels(OperationsMap objs, List<ModelMap> allModels) {
    OperationsMap results = super.postProcessOperationsWithModels(objs, allModels);

    OperationMap ops = results.getOperations();
    List<CodegenOperation> opList = ops.getOperation();
    for (CodegenOperation co : opList) {
      co.path = co.path.replaceAll("\\{\\w+\\}", "{}");
    }
    return results;
  }

  /**
   * Returns human-friendly help for the generator. Provide the consumer with help
   * tips, parameters here
   *
   * @return A string value for the help message
   */
  public String getHelp() {
    return "Generates a cpp-request client library.";
  }

  public CppRequestGenerator() {
    super();

    // set the output folder here
    outputFolder = "generated-code/cpp-request";

    modelTemplateFiles.put(
        "model-header.mustache",
        ".h");

    modelTemplateFiles.put(
        "model-body.mustache",
        ".cpp");
    /**
     * Api classes. You can write classes for each Api file with the
     * apiTemplateFiles map.
     * as with models, add multiple entries with different extensions for multiple
     * files per
     * class
     */
    apiTemplateFiles.put("api-header.mustache", ".h");
    apiTemplateFiles.put("api-body.mustache", ".cpp");

    /**
     * Template Location. This is the location which templates will be read from.
     * The generator
     * will use the resource stream to attempt to read the templates.
     */
    templateDir = "cpp-request";

    /**
     * Api Package. Optional, if needed, this can be used in templates
     */
    apiPackage = "org.openapitools.api";

    /**
     * Model Package. Optional, if needed, this can be used in templates
     */
    modelPackage = "org.openapitools.model";

    /**
     * Supporting Files. You can write single files for the generator with the
     * entire object tree available. If the input file has a suffix of `.mustache
     * it will be processed by the template engine. Otherwise, it will be copied
     */
    supportingFiles.add(new SupportingFile("CMakeLists.txt.mustache", "", "CMakeLists.txt"));
    supportingFiles.add(new SupportingFile("json.cpp.mustache", getSourceFolder().toString(), "json.cpp"));

    /**
     * Language Specific Primitives. These types will not trigger imports by
     * the client generator
     */
    languageSpecificPrimitives = new HashSet<String>(
        Arrays.asList(
            "Type1", // replace these with your types
            "Type2"));

    this.variableNameFirstCharacterUppercase = false;

    addOption(CodegenConstants.PACKAGE_NAME, "C++ package (library) name.", DEFAULT_PACKAGE_NAME);
    addOption(CodegenConstants.MODEL_PACKAGE, "C++ namespace for models (convention: name.space.model).",
        this.modelPackage);
    addOption(CodegenConstants.API_PACKAGE, "C++ namespace for apis (convention: name.space.api).",
        this.apiPackage);

    additionalProperties.put("apiVersion", apiVersion);
    additionalProperties.put("prefix", PREFIX);
    additionalProperties.put("namespace", "openapi");
    additionalProperties.put(CodegenConstants.PACKAGE_NAME, "openapi");
    additionalProperties.put(CodegenConstants.INTERFACE_PREFIX, "");
    additionalProperties.put(CodegenConstants.ENUM_CLASS_PREFIX, "");
    additionalProperties.put(CodegenConstants.API_NAME_PREFIX, "");
    additionalProperties.put(CodegenConstants.MODEL_NAME_PREFIX, PREFIX);

    typeMapping = new HashMap<>();
    typeMapping.put("date", "std::string");
    typeMapping.put("DateTime", "std::string");
    typeMapping.put("string", "std::string");
    typeMapping.put("integer", "std::int32_t");
    typeMapping.put("long", "std::int64_t");
    typeMapping.put("boolean", "bool");
    typeMapping.put("number", "double");
    typeMapping.put("Double", "double");
    typeMapping.put("array", "std::vector");
    typeMapping.put("map", "std::map");
    typeMapping.put("set", "std::set");
    typeMapping.put("string", "std::string");
    typeMapping.put("object", "rc<json_t>");
    typeMapping.put("ByteArray", "std::vector<std::byte>");
    typeMapping.put("UUID", "std::string");
    typeMapping.put("URI", "std::string");
    typeMapping.put("file", "std::vector<std::byte>");
    typeMapping.put("binary", "std::vector<std::byte>");
    typeMapping.put("AnyType", "rc<json_t>");
    typeMapping.put("null", "nullptr");

    systemIncludes.put("std::string", "string");
    systemIncludes.put("std::int32_t", "cstdint");
    systemIncludes.put("std::int64_t", "cstdint");
    systemIncludes.put("std::map", "map");
    systemIncludes.put("std::vector", "vector");
    systemIncludes.put("std::set", "set");
    systemIncludes.put("std::any", "any");
    systemIncludes.put("std::byte", "cstddef");
    systemIncludes.put("std::vector<std::byte>", "vector");
    systemIncludes.put("bool", "cstdint");
    systemIncludes.put("rc<json_t>", "memory");
  }

  @Override
  public void processOpts() {
    super.processOpts();
    supportingFiles.add(new SupportingFile("type.h.mustache", getIncludeFolder().toString(), "type.h"));
  }

  @Override
  @SuppressWarnings("rawtypes")
  public String getSchemaType(Schema p) {
    String openAPIType = super.getSchemaType(p);

    String type = null;
    if (typeMapping.containsKey(openAPIType)) {
      type = typeMapping.get(openAPIType);
      if (languageSpecificPrimitives.contains(type)) {
        return type;
      }
      if (foundationClasses.contains(type)) {
        return type;
      }
    } else {
      type = toModelName(openAPIType);
    }
    return type;
  }

  @Override
  public String getTypeDeclaration(String str) {
    return str;// toModelName(str);
  }

  @Override
  @SuppressWarnings("rawtypes")
  public String getTypeDeclaration(Schema p) {
    String openAPIType = getSchemaType(p);

    if (ModelUtils.isArraySchema(p)) {
      Schema inner = ModelUtils.getSchemaItems(p);
      return getSchemaType(p) + "<" + getTypeDeclaration(inner) + ">";
    } else if (ModelUtils.isMapSchema(p)) {
      Schema inner = ModelUtils.getAdditionalProperties(p);
      return getSchemaType(p) + "<std::string, " + getTypeDeclaration(inner) + ">";
    } else if (ModelUtils.isBinarySchema(p)) {
      return getSchemaType(p);
    } else if (ModelUtils.isFileSchema(p)) {
      return getSchemaType(p);
    }
    if (foundationClasses.contains(openAPIType)) {
      return openAPIType;
    } else if (languageSpecificPrimitives.contains(openAPIType)) {
      return openAPIType;
    } else {
      return openAPIType;
    }
  }

  @Override
  @SuppressWarnings("rawtypes")
  public String toDefaultValue(Schema p) {
    if (ModelUtils.isBooleanSchema(p)) {
      return "false";
    } else if (ModelUtils.isMapSchema(p)) {
      // Schema inner = ModelUtils.getAdditionalProperties(p);
      return "{}";
    } else if (ModelUtils.isArraySchema(p)) {
      // Schema inner = ModelUtils.getSchemaItems(p);
      return "{}";
    } else if (ModelUtils.isEnumSchema(p)) {
      return "(%s)0".formatted(getSchemaType(p));
    } else if (ModelUtils.isDateSchema(p)) {
      return "{}";
    } else if (ModelUtils.isDateTimeSchema(p)) {
      return "{}";
    } else if (ModelUtils.isNumberSchema(p)) {
      if (SchemaTypeUtil.FLOAT_FORMAT.equals(p.getFormat())) {
        return "0.0f";
      }
      return "0.0";
    } else if (ModelUtils.isIntegerSchema(p)) {
      if (SchemaTypeUtil.INTEGER64_FORMAT.equals(p.getFormat())) {
        return "0L";
      }
      return "0";
    } else if (ModelUtils.isStringSchema(p)) {
      return "{}";
    } else if (!StringUtils.isEmpty(p.get$ref())) {
      return toModelName(ModelUtils.getSimpleRef(p.get$ref())) + "()";
    }
    return "{}";
  }

  @Override
  public void postProcessModelProperty(CodegenModel model, CodegenProperty property) {
    if (model.classname == property.dataType) {
      String t = property.dataType;
      property.dataType = "rc<%s>".formatted(t);
      property.defaultValue = "nullptr";
    }
  }

  @Override
  public String toBooleanGetter(String name) {
    return "is" + getterAndSetterCapitalize(name);
  }

  @Override
  public String toModelImport(String name) {
    if (name.isEmpty()) {
      return null;
    }
    // if (namespaces.containsKey(name)) {
    // return "using " + namespaces.get(name) + ";";
    if (systemIncludes.containsKey(name)) {
      return "#include <" + systemIncludes.get(name) + ">";
    } else if (importMapping.containsKey(name)) {
      return importMapping.get(name);
    }

    Path p = getModelIncludeFolder().resolve(name + ".h");

    return "#include \"%s\"".formatted(p.subpath(1, p.getNameCount()));
  }

  @Override
  public boolean isDataTypeString(String dataType) {
    return "std::string".equals(dataType);
  }

  /**
   * Escapes a reserved word as defined in the `reservedWords` array. Handle
   * escaping
   * those terms here. This logic is only called if a variable matches the
   * reserved words
   *
   * @return the escaped term
   */
  @Override
  public String escapeReservedWord(String name) {
    return "_" + name; // add an underscore to the name
  }

  @Override
  public String modelFileFolder() {
    return outputFolder;
  }

  @Override
  public String apiFileFolder() {
    return outputFolder;
  }

  @Override
  public String apiFilename(String templateName, String tag) {
    return apiFilename(templateName, tag, apiFileFolder());
  }

  @Override
  public String apiFilename(String templateName, String tag, String outputDir) {
    String suffix = apiTemplateFiles().get(templateName);
    return Path.of(outputDir, getApiDstFromTemplateName(templateName).toString(), toApiFilename(tag) + suffix)
        .toString();
  }

  @Override
  public String modelFilename(String templateName, String modelName) {
    return modelFilename(templateName, modelName, modelFileFolder());
  }

  @Override
  public String modelFilename(String templateName, String modelName, String outputDir) {
    String suffix = modelTemplateFiles().get(templateName);
    return Path
        .of(outputDir, getModelDstFromTemplateName(templateName).toString(),
            getModelNamePrefix() + toModelFilename(modelName) + suffix)
        .toString();
  }

  /**
   * override with any special text escaping logic to handle unsafe
   * characters so as to avoid code injection
   *
   * @param input String to be cleaned up
   * @return string with unsafe characters removed or escaped
   */
  @Override
  public String escapeUnsafeCharacters(String input) {
    // TODO: check that this logic is safe to escape unsafe characters to avoid code
    // injection
    return input;
  }

  /**
   * Escape single and/or double quote to avoid code injection
   *
   * @param input String to be cleaned up
   * @return string with quotation mark removed or escaped
   */
  public String escapeQuotationMark(String input) {
    // TODO: check that this logic is safe to escape quotation mark to avoid code
    // injection
    return input.replace("\"", "\\\"");
  }
}
